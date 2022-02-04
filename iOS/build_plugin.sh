#!/bin/sh
echo ""
echo "Building plugin for iOS"

if [[ -z ${FFMPEG_KIT+empty} ]]; then 
    echo "Make sure the var FFMPEG_KIT is set to the directory of ffmpeg kit"
    exit 
fi

echo "FFMPEG_KIT: ${FFMPEG_KIT}"

xcodebuild build \
    -sdk iphonesimulator \
    -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj \
    ONLY_ACTIVE_ARCH=NO \
    VALID_ARCHS=x86_64 \
    ARCHS=x86_64 \
    -configuration "Release" \
        HEADER_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-ios/ffmpeg/include" \
        LIBRARY_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-ios/ffmpeg/lib" \
        OTHER_CFLAGS="-DDISABLE_LOGGING" \
        OTHER_LDFLAGS="-framework CoreFoundation -framework VideoToolbox -framework CoreVideo -framework CoreMedia -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswscale -lswresample -llibz -libbz2"

echo "SIMULATOR BUILD COMPLETE\n"

xcodebuild build \
    -sdk iphoneos \
    -project ./vol_unity_lib_ios/vol_unity_lib_ios.xcodeproj \
    ONLY_ACTIVE_ARCH=NO \
    VALID_ARCHS=arm64 \
    ARCHS=arm64 \
    -configuration "Release" \
        HEADER_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-ios/ffmpeg/include" \
        LIBRARY_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-ios/ffmpeg/lib" \
        OTHER_CFLAGS="-DDISABLE_LOGGING" \
        OTHER_LDFLAGS="-framework CoreFoundation -framework VideoToolbox -framework CoreVideo -framework CoreMedia -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswscale -lswresample -llibz -libbz2"
echo "DEVICE BUILD COMPLETE\n"

lipo -create \
    ./vol_unity_lib_ios/build/Release-iphoneos/libvol_unity_lib_ios.a \
    ./vol_unity_lib_ios/build/Release-iphonesimulator/libvol_unity_lib_ios.a \
    -output "./libVolAv.a"
echo "UNIVERSAL LIB CREATED\n"

rm -r ./vol_unity_lib_ios/build
echo "Cleaned up"

echo ""
echo "Done!"