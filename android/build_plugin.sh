#!/bin/sh
echo ""
echo "Building Android libs..."

if [[ -z ${FFMPEG_KIT+empty} ]]; then 
    echo "Make sure the var FFMPEG_KIT is set to the directory of ffmpeg kit"
    exit 
fi

echo "FFMPEG_KIT: ${FFMPEG_KIT}"

$ANDROID_NDK_ROOT/ndk-build \
    NDK_PROJECT_PATH=. \
    NDK_APPLICATION_MK=Application.mk \
    APP_CFLAGS=-DFFMPEG_KIT=${FFMPEG_KIT} \
    $*

echo ""
echo "Cleaning up / removing build folders..."  #optional..

mkdir -p ../VologramsToolkit/Plugins/Android

cp -r libs/arm64-v8a ../VologramsToolkit/Plugins/Android
cp -r libs/armeabi-v7a ../VologramsToolkit/Plugins/Android 
cp -r libs/x86 ../VologramsToolkit/Plugins/Android
cp -r libs/x86_64 ../VologramsToolkit/Plugins/Android

rm -rf libs
rm -rf obj

echo ""
echo "Done!"
