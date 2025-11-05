#!/bin/bash

# Create universal directory structure
mkdir -p "${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/lib"
mkdir -p "${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/include"

# List of libraries to combine
LIBS=(
    "libavcodec"
    "libavdevice"
    "libavfilter"
    "libavformat"
    "libavutil"
    "libswscale"
    "libswresample"
)

# Combine each library
for lib in "${LIBS[@]}"; do
    echo "Creating universal $lib..."
    lipo -create \
        "${FFMPEG_KIT}/prebuilt/apple-macos-arm64/ffmpeg/lib/${lib}.a" \
        "${FFMPEG_KIT}/prebuilt/apple-macos-x86_64/ffmpeg/lib/${lib}.a" \
        -output "${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/lib/${lib}.a"
done

# Copy headers (they're the same for both architectures)
echo "Copying headers..."
cp -R "${FFMPEG_KIT}/prebuilt/apple-macos-arm64/ffmpeg/include/*" "${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/include/"

echo "Done! Universal libraries are in ${FFMPEG_KIT}/prebuilt/bundle-apple-universal-macos/"
