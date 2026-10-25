// SPDX-License-Identifier: GPL-2.0-or-later
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#include <signal.h>

static OSStatus makeSpotifyEvent(const AEAddressDesc *target, AppleEvent *event) {
    AEDesc container={typeNull,NULL},property={typeNull,NULL},object={typeNull,NULL};
    OSType propertyCode='pPlS';
    OSStatus err=AECreateAppleEvent(kAECoreSuite,kAEGetData,target,kAutoGenerateReturnID,kAnyTransactionID,event);
    if(!err)err=AECreateDesc(typeType,&propertyCode,sizeof(propertyCode),&property);
    if(!err)err=CreateObjSpecifier(typeProperty,&container,formPropertyID,&property,false,&object);
    if(!err)err=AEPutParamDesc(event,keyDirectObject,&object);
    AEDisposeDesc(&object);AEDisposeDesc(&property);
    return err;
}

// Read only Spotify's player-state property, addressed to its existing process.
// A PID target cannot launch Spotify, and no title, artist, audio or keystrokes are read.
static NSDictionary *spotifyState(BOOL requestPermission) {
    NSArray<NSRunningApplication *> *apps=[NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.spotify.client"];
    if(!apps.count)return @{@"label":@"Spotify is closed",@"playing":@NO};
    pid_t pid=apps.firstObject.processIdentifier;AEAddressDesc target={typeNull,NULL};
    OSStatus err=AECreateDesc(typeKernelProcessID,&pid,sizeof(pid),&target);
    if(err!=noErr)return @{@"label":@"Spotify unavailable",@"playing":@NO};
    err=AEDeterminePermissionToAutomateTarget(&target,kAECoreSuite,kAEGetData,requestPermission);
    if(err!=noErr){AEDisposeDesc(&target);return @{@"label":@"Authorize Spotify from the menu",@"playing":@NO};}
    AppleEvent event={typeNull,NULL},reply={typeNull,NULL};
    err=makeSpotifyEvent(&target,&event);
    if(!err)err=AESendMessage(&event,&reply,kAEWaitReply|kAENeverInteract,60); // One-second reply timeout.
    SInt32 remoteError=0;Size actual=0;DescType type=typeNull;
    if(!err&&AEGetParamPtr(&reply,keyErrorNumber,typeSInt32,&type,&remoteError,sizeof(remoteError),&actual)==noErr)err=remoteError;
    OSType state=0;
    if(!err)err=AEGetParamPtr(&reply,keyDirectObject,typeEnumerated,&type,&state,sizeof(state),&actual);
    AEDisposeDesc(&event);AEDisposeDesc(&reply);AEDisposeDesc(&target);
    if(err||actual!=sizeof(state))return @{@"label":@"Spotify not responding",@"playing":@NO};
    if(state=='kPSP')return @{@"label":@"Spotify is playing",@"playing":@YES};
    if(state=='kPSp')return @{@"label":@"Spotify is paused",@"playing":@NO};
    return @{@"label":@"Spotify is stopped",@"playing":@NO};
}
static NSDictionary *runLink(NSNumber *music) {
    NSString *path=[[NSBundle mainBundle] pathForResource:@"station-link" ofType:nil];
    if(!path)return @{@"status":@"helper_missing"};
    NSTask *task=[NSTask new];task.executableURL=[NSURL fileURLWithPath:path];
    task.arguments=music?@[@"--music",music.boolValue?@"1":@"0"]:@[@"--probe"];
    NSPipe *pipe=[NSPipe pipe];task.standardOutput=pipe;task.standardError=[NSFileHandle fileHandleWithNullDevice];
    NSError *error=nil;if(![task launchAndReturnError:&error])return @{@"status":@"helper_failed"};
    NSTimeInterval deadline=[NSDate timeIntervalSinceReferenceDate]+3;
    while(task.running&&[NSDate timeIntervalSinceReferenceDate]<deadline)[NSThread sleepForTimeInterval:0.02];
    if(task.running){[task terminate];[NSThread sleepForTimeInterval:0.1];if(task.running)kill(task.processIdentifier,SIGKILL);}
    [task waitUntilExit];NSData *data=[pipe.fileHandleForReading readDataToEndOfFile];
    id decoded=[NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    return [decoded isKindOfClass:NSDictionary.class]?decoded:@{@"status":@"helper_failed"};
}
static NSString *keyboardLabel(NSString *s) {
    NSDictionary *labels=@{@"ready":@"Keyboard connected",@"music_updated":@"Keyboard connected",@"firmware_update_required":@"Keyboard needs the Field Music firmware",@"keyboard_disconnected":@"Keyboard disconnected",@"keyboard_busy":@"Keyboard busy — close Vial to reconnect",@"multiple_keyboards":@"More than one matching keyboard connected"};
    return labels[s]?:@"Keyboard connection unavailable";
}
@interface FrogStation : NSObject <NSApplicationDelegate>
@property NSStatusItem *item;
@property NSMenuItem *keyboardItem;
@property NSMenuItem *spotifyItem;
@property NSMenuItem *enabledItem;
@property NSTimer *timer;
@property BOOL enabled;
@property BOOL inFlight;
@property BOOL requestPermission;
@property id activity;
@property dispatch_queue_t worker;
@end
@implementation FrogStation
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    self.worker=dispatch_queue_create("local.frogstation.poll",DISPATCH_QUEUE_SERIAL);
    self.enabled=[[NSUserDefaults standardUserDefaults] boolForKey:@"SpotifyEnabled"];
    self.item=[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    NSImage *icon=[NSImage imageWithSystemSymbolName:@"leaf" accessibilityDescription:@"Frog Station"];
    if(icon){icon.template=YES;self.item.button.image=icon;}else self.item.button.title=@"Frog";
    self.item.button.toolTip=@"Frog Station";
    NSMenu *menu=[NSMenu new];
    self.keyboardItem=[menu addItemWithTitle:@"Checking keyboard…" action:nil keyEquivalent:@""];
    self.spotifyItem=[menu addItemWithTitle:@"Spotify music mode is off" action:nil keyEquivalent:@""];
    [menu addItem:NSMenuItem.separatorItem];
    self.enabledItem=[menu addItemWithTitle:@"Spotify music mode" action:@selector(toggle:) keyEquivalent:@""];self.enabledItem.target=self;
    NSMenuItem *authorize=[menu addItemWithTitle:@"Authorize Spotify…" action:@selector(authorize:) keyEquivalent:@""];authorize.target=self;
    [menu addItem:NSMenuItem.separatorItem];
    NSMenuItem *quit=[menu addItemWithTitle:@"Quit Frog Station" action:@selector(quit:) keyEquivalent:@"q"];quit.target=self;
    self.item.menu=menu;[self updateCheck];
    // Continue refreshing the short firmware lease while the status menu is open.
    self.timer=[NSTimer timerWithTimeInterval:2 target:self selector:@selector(tick:) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop] addTimer:self.timer forMode:NSRunLoopCommonModes];
    [self tick:nil];
}
- (void)updateCheck {
    self.enabledItem.state=self.enabled?NSControlStateValueOn:NSControlStateValueOff;
    if(self.enabled&&!self.activity)self.activity=[[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep reason:@"Sync Spotify playback state to the keyboard"];
    if(!self.enabled&&self.activity){[[NSProcessInfo processInfo] endActivity:self.activity];self.activity=nil;}
}
- (void)toggle:(id)sender {
    (void)sender;self.enabled=!self.enabled;self.requestPermission=self.enabled;
    [[NSUserDefaults standardUserDefaults] setBool:self.enabled forKey:@"SpotifyEnabled"];[self updateCheck];[self tick:nil];
}
- (void)authorize:(id)sender {
    (void)sender;self.enabled=YES;self.requestPermission=YES;
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"SpotifyEnabled"];[self updateCheck];[self tick:nil];
}
- (void)tick:(id)sender {
    (void)sender;if(self.inFlight)return;self.inFlight=YES;
    BOOL enabled=self.enabled,ask=self.requestPermission;self.requestPermission=NO;
    dispatch_async(self.worker,^{ @autoreleasepool {
        NSDictionary *spotify=enabled?spotifyState(ask):@{@"label":@"Spotify music mode is off",@"playing":@NO};
        NSDictionary *link=runLink(@(enabled&&[spotify[@"playing"] boolValue]));
        dispatch_async(dispatch_get_main_queue(),^{
            self.inFlight=NO;self.keyboardItem.title=keyboardLabel(link[@"status"]);
            self.spotifyItem.title=spotify[@"label"];
            // A user toggle made during a poll takes effect promptly.
            if(self.enabled!=enabled||self.requestPermission)[self tick:nil];
        });
    }});
}
- (void)quit:(id)sender {
    (void)sender;[self.timer invalidate];
    // The eight-second firmware lease expires independently, including after crashes.
    [NSApp terminate:nil];
}
@end
int main(int argc,const char **argv) { @autoreleasepool {
    if(argc==2&&!strcmp(argv[1],"--self-test")) {
        if(![keyboardLabel(@"firmware_update_required") containsString:@"Field Music"]||![keyboardLabel(@"ready") isEqualToString:@"Keyboard connected"])return 1;
        pid_t pid=getpid();AEAddressDesc target={typeNull,NULL};AppleEvent event={typeNull,NULL};
        if(AECreateDesc(typeKernelProcessID,&pid,sizeof(pid),&target)||makeSpotifyEvent(&target,&event))return 1;
        AEEventID eventID=0;DescType type=0;Size size=0;
        if(AEGetAttributePtr(&event,keyEventIDAttr,typeType,&type,&eventID,sizeof(eventID),&size)||eventID!=kAEGetData)return 1;
        AEDesc object={typeNull,NULL};OSType property=0;
        if(AEGetParamDesc(&event,keyDirectObject,typeObjectSpecifier,&object)||AEGetKeyPtr(&object,keyAEKeyData,typeType,&type,&property,sizeof(property),&size)||property!='pPlS')return 1;
        AEDisposeDesc(&object);AEDisposeDesc(&event);AEDisposeDesc(&target);
        puts("PASS: status mapping and PID-addressed read-only Spotify property event construction; no event was sent and no USB, permission or preference operations occurred.");return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--probe")){
        NSData *d=[NSJSONSerialization dataWithJSONObject:runLink(nil) options:0 error:nil];fwrite(d.bytes,1,d.length,stdout);putchar('\n');return 0;
    }
    [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    FrogStation *delegate=[FrogStation new];NSApp.delegate=delegate;[NSApp run];
}}
