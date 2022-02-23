# Volograms Unity SDK 

# Prebuilt

Prebuilt versions of the Unity plugins are packaged into a `unitypackage` file in the [releases page](https://github.com/Volograms/volograms_unity_plugin/releases).

This repo contains the projects to build the native Volograms player plugins for Unity. It also uses code taken from [this repository](https://bitbucket.org/volograms/vol_av/src/master/). Each platform has a separate project and each project contains a README that documents how to build their respective plugin. When a plugin is built, it is copied to the `UnityVol` folder, which can be dragged and dropped into Unity. 

## Platform Avaialble 
- Mac (OSX, excluding M1 Macs)
- iOS
- Windows
- Android (WIP, expect issues)

## TODO Platforms 
- M1 Macs
- Linux

## Getting started

### Requirements 

#### iOS and MacOS

To build the volplayer library for iOS and MacOS, you must have the [Xcode Command Line Tools](https://mac.install.guide/commandlinetools/3.html) installed. Xcode itself is NOT necessary.

#### Android 

To build the volplayer library for Android, you must have the [Android SDK Command Line Tools](https://developer.android.com/studio/command-line) installed. Android Studio is NOT necessary.

#### Windows

The volplayer Windows build is a Visual Studio project, and so you must be able to build such a project, for example, through [Visual Studio](https://visualstudio.microsoft.com/).

### FFMPEG
The Volograms Unity SDK uses builds of [FFMPEG](https://www.ffmpeg.org/) created from the [ffmpeg-kit repo](https://github.com/tanersener/ffmpeg-kit). To start developing on the Volograms Unity SDK, clone the ffmpeg-kit repo and follow the instructions on the repo page to build ffmpeg for the platforms you want. 

Next, open your terminal and add an environmental variable `FFMPEG_KIT` which should point to the root folder of your local ffmpeg-kit repo. For example:
```
export FFMPEG_KIT="path/to/ffmpeg-kit"
```

Then all you have to do is run the `build_plugin.sh` script in any of the platform folders.

Note there is currently no `build_plugin.sh` script for Windows. 

## Unity Settings
The libraries are exported into the [`VologramsToolkit`](./VologramsToolkit/Plugins/) folder. The `VologramsToolkit` folder can be imported directly into Unity. 

After importing the plugin into Unity, ensure that the Unity settings for each plugin is correct. For example, we don't want to the iOS plugin in a Mac build. 

There is a utility script included in the Unity package that re-imports all the libraries into Unity with the correct settings. This can be done in Unity through the toolbar: `Volograms > Utils > Reimport Plugins > All`
