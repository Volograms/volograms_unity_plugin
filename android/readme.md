# Building the Unity Native Plugin for Android

## Setting Up

### FFmpeg
The Volograms Unity SDK uses the [ffmpeg-android-maker](https://github.com/Javernaut/ffmpeg-android-maker) tool to build FFmpeg and the plugin for Android. There is also a docker image available [here](https://github.com/Javernaut/ffmpeg-android-maker-docker) that can be used for the build process.

1. Clone ffmpeg-android-maker github repository into the `shared/ffmpeg` folder
2. Download the docker image `docker pull javernaut/ffmpeg-android-maker`
3. Build FFmpeg for Android from the `shared/ffmpeg/ffmpeg-android-maker` folder using the command
```shell
docker run --rm -v /path/to/ffmpeg-android-maker:/mnt/ffmpeg-android-maker javernaut/ffmpeg-android-maker
```
4. Build the plugin using the command `./build_plugin.sh` from the `android` folder. The docker image for ffmpeg-android-maker can be used to build the plugin. For example:
```shell
docker run --rm -e FFMPEG_KIT=/plugin/shared/ffmpeg/ffmpeg-android-maker/build/ffmpeg -v /path/to/unity_plugin_root:/plugin -w /plugin/android javernaut/ffmpeg-android-maker ./build_plugin.sh
```

<!-- ### Android SDK
The Volograms Unity SDK uses the Android SDK to build the plugin. You can download the Android SDK from [here](https://developer.android.com/studio#downloads). -->

