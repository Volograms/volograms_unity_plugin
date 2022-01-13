#!/bin/sh
echo ""
echo "Building plugin for iOS"

SIM_TARGET_NAME=$( xcodebuild -scheme vol_unity_lib_ios -sdk iphonesimulator -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj -destination 'platform=iOS Simulator,OS=14.0.1,name=iPad (8th generation)' -showBuildSettings | grep FULL_PRODUCT_NAME | grep -oE '[^ ]+$' )
SIM_PRODUCT_DIR=$( xcodebuild -scheme vol_unity_lib_ios -sdk iphonesimulator -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj -destination 'platform=iOS Simulator,OS=14.0.1,name=iPad (8th generation)' -showBuildSettings | grep TARGET_BUILD_DIR | grep -oEi "\/.*" )
SIM_PRODUCT="${SIM_PRODUCT_DIR}/${SIM_TARGET_NAME}"
echo $SIM_PRODUCT
xcodebuild build -quiet -scheme vol_unity_lib_ios -sdk iphonesimulator -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj -destination 'platform=iOS Simulator,OS=14.0.1,name=iPad (8th generation)' 
echo "SIMULATOR BUILD COMPLETE\n"

DEV_TARGET_NAME=$( xcodebuild -sdk iphoneos -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj -destination 'platform=iOS,name=Any iOS Device' -showBuildSettings | grep FULL_PRODUCT_NAME | grep -oE '[^ ]+$' )
DEV_PRODUCT_DIR=$( xcodebuild -sdk iphoneos -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj -destination 'platform=iOS,name=Any iOS Device' -showBuildSettings | grep TARGET_BUILD_DIR | grep -oEi "\/.*" )
DEV_PRODUCT="${DEV_PRODUCT_DIR}/${DEV_TARGET_NAME}"
echo $DEV_PRODUCT
xcodebuild build -quiet -sdk iphoneos -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj -destination 'platform=iOS,name=Any iOS Device'
echo "DEVICE BUILD COMPLETE\n"

lipo -create $DEV_PRODUCT $SIM_PRODUCT -output "./libVolAv.a"
echo "UNIVERSAL LIB CREATED\n"

rm -r ./vol_unity_lib_ios/build
echo "Cleaned up"

echo ""
echo "Done!"