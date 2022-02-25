# Volograms Unity SDK

This is the [Volograms](https://www.volograms.com/) SDK for Unity, in the form of a plugin. You can use this plugin to create an app that plays volograms.

## Supported Platforms 

* macOS - Excluding M1 Macs.
* iOS
* Windows
* Android - Work-in-progress, expect issues!

Support is planned for M1 macOS and GNU/Linux builds.

## Getting Started

* The easiest way to get started using is to download a pre-built plugin from the [Releases](https://github.com/Volograms/volograms_unity_plugin/releases) page.
* If you would prefer to build the plugin from source, then you can clone this respository, and follow the [Developer Guide](#developer-guide).

## Unity Settings

The libraries are exported into the [`VologramsToolkit`](./VologramsToolkit/Plugins/) folder. The `VologramsToolkit` folder can be imported directly into Unity. 

After importing the plugin into Unity, ensure that the Unity settings for each plugin is correct. For example, we don't want to use the iOS plugin in a macOS build. 

There is a utility script included in the Unity package that re-imports all the libraries into Unity with the correct settings.
This can be invoked in Unity through the toolbar: `Volograms > Utils > Reimport Plugins > All`.

## Developer Guide

Each platform supported has a separate project, and each project contains a `README` that documents how to build its respective plugin.
When a plugin is built, it is copied to the `UnityVol` folder, which can be dragged and dropped into Unity.

### Requirements 

#### iOS and MacOS

To build the _volplayer_ library for iOS and macOS, you must have the [Xcode Command Line Tools](https://mac.install.guide/commandlinetools/3.html) installed. Xcode itself is NOT necessary.

#### Android 

To build the _volplayer_ library for Android, you must have the [Android SDK Command Line Tools](https://developer.android.com/studio/command-line) installed. Android Studio is NOT necessary.

#### Windows

The _volplayer_ Windows build is a Visual Studio project, and so you must be able to build such a project, for example, through [Visual Studio](https://visualstudio.microsoft.com/).

### FFmpeg

The Volograms Unity SDK uses builds of [FFmpeg](https://www.ffmpeg.org/) created from the [ffmpeg-kit repo](https://github.com/tanersener/ffmpeg-kit).
To start developing on the Volograms Unity SDK, clone the `ffmpeg-kit` repository and follow the instructions to build FFmpeg for the platforms you want. 

Next, open your terminal and add an environment variable `FFMPEG_KIT` which should point to the root folder of your local ffmpeg-kit repository.
For example:
```
export FFMPEG_KIT="/path/to/ffmpeg-kit"
```

Then, all you have to do is run the `build_plugin.sh` script in any of the platform folders.

Note there is currently no `build_plugin.sh` script for Windows. 

## Licence ##

Copyright 2022, Volograms.

* Source code included in this plugin is provided under the terms of the MIT licence (see the `LICENSE` file for details).

### Dependencies

* This software uses <a href=http://ffmpeg.org>FFmpeg</a> licensed under the <a href=http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>LGPLv2.1</a>.
* The source code distribution, build instructions, and further licence details can be found at [ffmpeg-kit repo](https://github.com/tanersener/ffmpeg-kit)
