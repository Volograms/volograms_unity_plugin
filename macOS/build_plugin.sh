#!/bin/sh
echo ""
echo "Building plugin for Mac"

DEV_TARGET_NAME=$( xcodebuild -scheme vol_unity_macos -project ./UnityPlugin/vol_unity_macos.xcodeproj -destination 'platform=macOS' -showBuildSettings | grep FULL_PRODUCT_NAME | grep -oE '[^ ]+$' )
DEV_PRODUCT_DIR=$( xcodebuild -scheme vol_unity_macos -project ./UnityPlugin/vol_unity_macos.xcodeproj -destination 'platform=macOS' -showBuildSettings | grep TARGET_BUILD_DIR | grep -oEi "\/.*" )
DEV_PRODUCT="${DEV_PRODUCT_DIR}/${DEV_TARGET_NAME}"
xcodebuild build -quiet -scheme vol_unity_macos -project ./UnityPlugin/vol_unity_macos.xcodeproj -destination 'platform=macOS'

cp -R ${DEV_PRODUCT} ./

echo ""
echo "Done!"