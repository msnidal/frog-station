#!/bin/sh
set -eu
task_helper=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_app="$task_helper/../build/Frog Station.app"
mkdir -p "$task_app/Contents/MacOS" "$task_app/Contents/Resources"
xcrun clang -arch arm64 -arch x86_64 -mmacosx-version-min=13.0 -O2 -Wall -Wextra -Werror "$task_helper/station_link.c" -framework CoreFoundation -framework IOKit -o "$task_app/Contents/Resources/station-link"
xcrun clang -arch arm64 -arch x86_64 -mmacosx-version-min=13.0 -fobjc-arc -O2 -Wall -Wextra -Werror -Wno-multichar "$task_helper/FrogStation.m" -framework Cocoa -framework Carbon -o "$task_app/Contents/MacOS/FrogStation"
cp "$task_helper/Info.plist" "$task_app/Contents/Info.plist"
codesign --force --sign - "$task_app/Contents/Resources/station-link"
codesign --force --sign - --options runtime --entitlements "$task_helper/entitlements.plist" "$task_app"
codesign --verify --deep --strict "$task_app"
