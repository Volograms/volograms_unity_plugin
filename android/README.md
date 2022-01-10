# Building the Unity Native Plugin for Android (WIP)

## Project Layout 
* [`src folder`](./src/) contains all the files needed to build the Android plugin. 
* It includes Android make files that builds the plugin for the following architectures: `arm64-v8a`, `armeabi-v7a`, `x86` and `x86_64`
* You do NOT need Android Studio but you do need the [Android SDK tools (scroll down to "Command line tools only")](https://developer.android.com/studio#downloads)
* You also need Android NDK, the specific version being used can be downloaded using the `sdkmanager` downloaded from the SDK tools: `sdkmanager --install "ndk;19.0.5232133"`

## FFMPEG
The last thing needed is an ffmpeg build for Android. (ffmpeg kit)[https://github.com/tanersener/ffmpeg-kit] is a handy repo for building ffmpeg on different platforms. 

* Clone the repo 
* Access the repo from the command line 
* Run the command `./android.sh --enable-android-media-codec` 

## Build settings 

In the [Android app make file](./src/Android.mk) make sure `FFMPEG_PATH` points to the ffmpeg `.so` files, using `$(TARGET_ARC_ABI)` to point to the correct architecture. 

## Run the build

Run `./build-plugin.sh` to build the Unity plugin 

## Add plugin to Unity 

* Open/Create your Unity project 
* In the project pane create a `Plugins/Android/` folder where desired under `Assets` e.g. `Assets/Plugins/Android/` OR `Assets/Volograms/Plugins/Android/`
* Import the arch builds you want from [`Plugin Build`](./src/libs/) into your `Plugins/Android/` folder
* For each `.so` file under `Plugins/Android/*/` in Unity make sure: 
    * The "Select platforms for plugin" window has only the Android checkbox selected 
    * The CPU matches the architecure folder containing the `.so`:
        * `arm64-v8a` = `ARM64`
        * `armeabi-v7a` = `ARMv7`
        * `x86` = `x86`
* The Unity project is ready to build 