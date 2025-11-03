#!/bin/sh
echo ""
echo "Building Android libs..."

PLUGIN_ROOT="$(pwd)/../"

if [ -z "$FFMPEG_KIT" ]; then 
    echo "Make sure the var FFMPEG_KIT is set to the directory of ffmpeg build output"
    exit 
fi

echo "FFMPEG_KIT: ${FFMPEG_KIT}"

$ANDROID_NDK_HOME/ndk-build \
    NDK_PROJECT_PATH=$PLUGIN_ROOT/android \
    NDK_APPLICATION_MK=Application.mk \
    APP_CFLAGS=-DFFMPEG_KIT=${FFMPEG_KIT} \
    $*

echo ""
echo "Cleaning up / removing build folders..."  #optional..

TARGET_DIR="$PLUGIN_ROOT/VologramsPlayer/Runtime/Plugins/Android/"

mkdir -p $TARGET_DIR

cp -r libs/arm64-v8a $TARGET_DIR
cp -r libs/armeabi-v7a $TARGET_DIR
cp -r libs/x86 $TARGET_DIR
cp -r libs/x86_64 $TARGET_DIR

rm -rf libs
rm -rf obj

echo ""
echo "Done!"
