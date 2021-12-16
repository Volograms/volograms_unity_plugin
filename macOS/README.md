# Building the Unity Native Plugin for Mac (OSX)

Since Xcode projects can only be built on Apple device, this guide assumes the use of a computer running OSX.

## Setting Up
* Ensure you have [`brew` (Homebrew)](https://brew.sh/) installed on your machine. 
* Using Homebrew, install [`ffmpeg`](https://ffmpeg.org/)
    * `brew install ffmpeg`
    * `ffmpeg` should be installed in the directory `/usr/local/Cellar/ffmpeg/{version}/`
* The Homebrew installation should have everything you need to build the OSX plugin.

## Project Layout
* Open `vol_unity_macos.xcodeproj`
* The project hierarchy looks like this:

![Project Hierarchy](/readme_resources/image-07.png)

* **IMPORTANT**: Take note that the `src` folder is actually the [`shared/src`](/shared/src/) folder, and all files contained in this folder are used by all platforms. Changing these files will affect other platforms' projects.

## Build Settings
* Access the **target** build settings
    * Click the `vol_unity_lib_ios` project in project navigator
    * Select `Build Settings` on the top toolbar 
    * On the left-hand side of the main pane, under `Targets`, select the `vol_unity_lib_ios` lib target 

![Build Settings](/readme_resources/image-08.png)

* Make sure the following settings have the following values:

| Build Settings Key                               | Build Setting Value |
| ------------------------------------------------ | ------------------- |
| `Architectures > Base SDK`                       | `macOS`             |
| `Architectures > Build Active Architecture Only` | `Yes`               |


The following build settings have a list of values:

| Build Settings Key                    | Build Setting Values                                |
| ------------------------------------- | --------------------------------------------------- |
| `Search Paths > Header Search Paths`  | `/usr/local/Cellar/ffmpeg/{ffmpeg_version}/include` |
| `Search Paths > Library Search Paths` | `$(inherited)`                                      |
|                                       | `/usr/local/Cellar/ffmpeg/4.4_2/lib`                |

All the entries for the `Search Paths` are **non-recursive**.

![Header Search Paths](/readme_resources/image-09.png)

![Library Search Paths](/readme_resources/image-10.png)

## Build Phases
* In the Target Settings the Build Phases look like this: 

![Build Phases](/readme_resources/image-11.png)

* The build phases should look like this by default, but if not:
    * Ensure the `Compile Sources` section contains `main.c`, `vol_geom.c` and `vol_av.c`.
    * Esnure the following `ffmpeg` libraries are in both the `Link Binary with Libraries` and `Embed Libraries` sections:
        * `libavcodec.xx.xxx.xxx.dylib`
        * `libavutil.xx.xxx.xxx.dylib`
        * `libavformat.xx.xxx.xxx.dylib`
        * `libswscale.xx.xxx.xxx.dylib`
        * `libavdevice.xx.xxx.xxx.dylib`
    * The `.dylib` files can be found at the `ffmpeg` install location (`/usr/local/Cellar/ffmpeg/{ffmpeg_version}/lib/`)

* There is a custom `Run Script` build phase that runs at the end of the build:

```shell
PRODUCT="${BUILT_PRODUCTS_DIR}/${TARGET_NAME}.bundle"
mkdir -p "${PROJECT_DIR}/../../UnityVol/Plugins/OSX/"
cp -r -f "${PRODUCT}" "${PROJECT_DIR}/../../UnityVol/Plugins/OSX/"
```

* The script grabs the built bundle and copies it to the `UnityVol` folder. This folder can be imported directly into Unity. 
* Before building make sure the build is using the `Release` build configuration. 
    * Edit the target's scheme by clicking the build target in the top toolbar and selecting `Edit Scheme`
    ![Edit Scheme](/readme_resources/image-12.png)

    * Ensure the `Run` `Build Configuration` is set to `Release`.
    ![Build Configuration](/readme_resources/image-13.png)

The built `.bundle` will be available at `UnityVol/Plugins/MacOS/vol_unity_macos.bundle`