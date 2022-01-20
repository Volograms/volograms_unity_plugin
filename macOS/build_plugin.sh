#!/bin/sh
echo "\nBuilding plugin for Mac"

if [[ -z ${FFMPEG_KIT+empty} ]]; then 
    echo "Make sure the var FFMPEG_KIT is set to the directory of ffmpeg kit"
    exit 
fi

echo "FFMPEG_KIT: ${FFMPEG_KIT}"

xcodebuild clean -scheme vol_unity_macos -project ./UnityPlugin/vol_unity_macos.xcodeproj -destination 'platform=macOS'

DEV_TARGET_NAME=$( xcodebuild -scheme vol_unity_macos -project ./UnityPlugin/vol_unity_macos.xcodeproj -destination 'platform=macOS' -showBuildSettings | grep FULL_PRODUCT_NAME | grep -oE '[^ ]+$' )
DEV_PRODUCT_DIR=$( xcodebuild -scheme vol_unity_macos -project ./UnityPlugin/vol_unity_macos.xcodeproj -destination 'platform=macOS' -showBuildSettings | grep TARGET_BUILD_DIR | grep -oEi "\/.*" )
DEV_PRODUCT="${DEV_PRODUCT_DIR}/${DEV_TARGET_NAME}"
xcodebuild build \
    -quiet \
    -scheme vol_unity_macos \
    -project ./UnityPlugin/vol_unity_macos.xcodeproj \
    -destination 'platform=macOS' \
    -configuration "Release" \
        HEADER_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/ffmpeg/include" \
        LIBRARY_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/ffmpeg/lib" \
        OTHER_LDFLAGS="-framework CoreFoundation -framework VideoToolbox -framework CoreVideo -framework CoreMedia -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswscale -lswresample"

cp -R ${DEV_PRODUCT} ./

echo ""
echo "\nDone!"