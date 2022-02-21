#!/bin/sh
echo "\nBuilding plugin for Mac"

if [[ -z ${FFMPEG_KIT+empty} ]]; then 
    echo "Make sure the var FFMPEG_KIT is set to the directory of ffmpeg kit"
    exit 
fi

echo "FFMPEG_KIT: ${FFMPEG_KIT}"

xcodebuild clean build \
    -quiet \
    -project ./UnityPlugin/vol_unity_macos.xcodeproj \
    ONLY_ACTIVE_ARCH=NO \
    VALID_ARCHS=x86_64 \
    ARCHS=x86_64 \
    -arch x86_64 \
    -configuration "Release" \
    HEADER_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/ffmpeg/include" \
    LIBRARY_SEARCH_PATHS="${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/ffmpeg/lib" \
    OTHER_CFLAGS="-DDISABLE_LOGGING" \
    OTHER_LDFLAGS="-framework CoreFoundation -framework VideoToolbox -framework CoreVideo -framework CoreMedia -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswscale -lswresample"

mkdir -p ../VologramsToolkit/Plugins/MacOS

cp -R ./UnityPlugin/build/Release/volplayer.bundle ../VologramsToolkit/Plugins/MacOS/

rm -r ./UnityPlugin/build/

echo ""
echo "\nDone!"