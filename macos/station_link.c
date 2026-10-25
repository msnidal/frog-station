// SPDX-License-Identifier: GPL-2.0-or-later
// Only identity GET, station GET and the leased music command exist here.
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stdio.h>
#include <stdlib.h>
#include "link_protocol.h"
static uint8_t reply[32],input[64];static bool waiting;static unsigned expected;
static void receive(void *ctx,IOReturn result,void *sender,IOHIDReportType type,uint32_t id,uint8_t *report,CFIndex length){
    (void)ctx;(void)sender;(void)type;(void)id;
    if(!waiting||result!=kIOReturnSuccess||length!=32)return;
    bool match=expected==0?station_identity_ok(report):report[0]==0xf1&&report[1]=='F'&&report[2]=='S'&&report[3]==1&&report[4]==expected-1;
    if(match){memcpy(reply,report,32);waiting=false;}
}
static bool query(IOHIDDeviceRef d,const uint8_t p[32],unsigned kind){
    expected=kind;waiting=true;memset(reply,0,32);
    IOReturn r=IOHIDDeviceSetReport(d,kIOHIDReportTypeOutput,0,p,32);
    if(r!=kIOReturnSuccess){waiting=false;return false;}
    CFAbsoluteTime until=CFAbsoluteTimeGetCurrent()+0.40;
    while(waiting&&CFAbsoluteTimeGetCurrent()<until)CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,true);
    bool got=!waiting;waiting=false;return got;
}
static void json(const char *status,int revision,int music){printf("{\"status\":\"%s\",\"interface_revision\":%d,\"music_active\":%s}\n",status,revision,music?"true":"false");}
int main(int argc,char **argv){
    bool probe=argc==2&&!strcmp(argv[1],"--probe");
    bool set=argc==3&&!strcmp(argv[1],"--music")&&(!strcmp(argv[2],"0")||!strcmp(argv[2],"1"));
    if(!probe&&!set){fprintf(stderr,"Usage: station-link --probe | --music 0|1\n");return 2;}
    const void *keys[]={CFSTR(kIOHIDVendorIDKey),CFSTR(kIOHIDProductIDKey),CFSTR(kIOHIDPrimaryUsagePageKey),CFSTR(kIOHIDPrimaryUsageKey)};
    int values[]={0x04d8,0xeb2d,0xff60,0x61};CFNumberRef nums[4];
    for(int i=0;i<4;i++)nums[i]=CFNumberCreate(NULL,kCFNumberIntType,&values[i]);
    CFDictionaryRef match=CFDictionaryCreate(NULL,keys,(const void **)nums,4,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    IOHIDManagerRef manager=IOHIDManagerCreate(NULL,kIOHIDOptionsTypeNone);IOHIDManagerSetDeviceMatching(manager,match);
    CFRelease(match);for(int i=0;i<4;i++)CFRelease(nums[i]);
    CFSetRef devices=IOHIDManagerCopyDevices(manager);CFIndex count=devices?CFSetGetCount(devices):0;
    if(count!=1){json(count?"multiple_keyboards":"keyboard_disconnected",0,0);if(devices)CFRelease(devices);CFRelease(manager);return 0;}
    IOHIDDeviceRef d;CFSetGetValues(devices,(const void **)&d);
    if(IOHIDDeviceOpen(d,kIOHIDOptionsTypeNone)!=kIOReturnSuccess){json("keyboard_busy",0,0);CFRelease(devices);CFRelease(manager);return 0;}
    IOHIDDeviceRegisterInputReportCallback(d,input,sizeof(input),receive,NULL);IOHIDDeviceScheduleWithRunLoop(d,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
    const char *status="identity_check_failed";int revision=0,music=0;uint8_t id[32]={0xfe,0},get[32]={0xf1,'F','S',1,0};
    if(query(d,id,0)&&station_identity_ok(reply)){
        status="station_check_failed";
        if(query(d,get,1)&&station_status_ok(reply,0)){
            revision=reply[10]|reply[11]<<8;music=reply[13];status="firmware_update_required";
            if(station_music_capable(reply)){
                status="ready";
                if(set){uint8_t p[32];station_music_packet(p,argv[2][0]=='1');
                    status="music_command_failed";
                    if(query(d,p,3)&&station_status_ok(reply,2)&&reply[13]==(uint8_t)(argv[2][0]=='1')){status="music_updated";music=reply[13];}
                }
            }
        }
    }
    IOHIDDeviceUnscheduleFromRunLoop(d,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);IOHIDDeviceClose(d,kIOHIDOptionsTypeNone);
    CFRelease(devices);CFRelease(manager);json(status,revision,music);return 0;
}
