# Building the Unity Native Plugin for Windows

Work in progress 

## Setting Up
* Download [`ffmpeg`](https://ffmpeg.org/) from this [repo](https://github.com/BtbN/FFmpeg-Builds/releases)
      * Download the build labelled like `ffmpeg-N-xxxxxx-xxxxxxxxxxx-win64-gpl-shared.zip`
      * Unzip the file and note where you place the extracted folder
      * In this document `ffmpeg_win` refers to the location of ffmpeg on your machine  

## Project Layout
* Open the `vol_unity_lib_win.sln` solution in Visual Studio
* The project hierarchy looks like this:

![Win Project Hierarchy](/readme_resources/image-19.png)

* **IMPORTANT** __ALL__ the files under `Header Files` and `Source Files` are from the [`shared/src`](/shared/src/) folder. These files are used by all platforms so be careful when making changes to these files. 

## Project Properties 
* First make sure the solution platform is `x64` and the configuration is set to `Release`

![Solution Platform & Configuration](/readme_resources/image-20.png)


### Setting the ffmpeg path
* Open the Property Manager window
      * `View` > `Other Windows` > `Property Window` 
* In the Property Manager window, expand the `Release | x64` 

![Property Manager Window](/readme_resources/image-21.png)

* Right-click the `vol_unity_win_proerty_sheet`
* Select `Properties` 
* In the left-hand pane, find `User Macros` under `Common Properties` 
* You should see a Macro named `FFMPEG_WIN`, double click it to edit it
* In the value field, enter the **absolute** path to the `ffmpeg_win` folder 

### Configuring the VS project 
* In the properties for the `vol_unity_lib_win` VS project:
* Under `Configuration Properties` > `C/C++` > `General`
      * Add `$(FFMPEG_WIN)/include` to `Additional Include Directories`
* Under `Configuration Properties` > `C/C++` > `Precompiled Headers` 
      * Ensure the `Precompiled Header` is set to `Not Using Precompiled Headers`
      * Ensure the `Precompiled Header File` is blank
* Under `Configuration Properties` > `Linker` > `General` 
      * Add `$(FFMPEG_WIN)/lib` to `Additional Library Directories`
* Under `Configuration Properties` > `Linker` > `Input`
      * Add the following values to `Additional Dependencies`:
            * avcodec.lib
            * avutil.lib
            * avformat.lib
            * swscale.lib
            * avdevice.lib
* Under `Configuration Properties` > `Build Events` > `Post Build Event` 
      * Make sure the `Command Line` field has the following value:
      * `call $(ProjectDir)..\Scripts\post-build.bat $(ProjectDir) $(TargetFilename) $(TargetPath) $(FFMPEG_WIN)`
      * The [post-build script](/windows/vol_unity_lib_win/Scripts/post-build.bat) copies the built dll into the Unity plugin folder.


## Post-build
* After the dll has been built and moved to the Unity folder (`UnityVol/Plugins/x64/`) the dll's dependencies must be added to that folder. 
* You can find them in the `ffmpeg_win/bin/` folder.
* The [post-build script](/windows/vol_unity_lib_win/Scripts/post-build.bat) should copy all the dependencies. 
* In case you need to check ensure the following files are in the `UnityVol/Plugins/x64/` folder after the build
      * `avcodec-xx.dll`
      * `avformat-xx.dll`
      * `avutil-xx.dll`
      * `swresample-xx.dll`
      * `swscale-xx.dll`