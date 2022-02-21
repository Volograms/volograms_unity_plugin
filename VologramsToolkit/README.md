# Volograms Unity Toolkit 
Version 1.11

## Supported Platforms 
* iOS
* MacOS (excluding Macs with M1 chips)
* Windows 
* Android (WORK IN PROGRESS, EXPECT ISSUES)

### Android 
The Android version of the player is still a work in progress. Please expect performance issues and no audio. These issues are currently being worked on. 

## `Vol Player` Component Inspector

| Field Name                        | Value Type    | Value Desc |
| ---                               | ---           | ---        |
| **Paths**                         |               |   |
| `Vol Folder > Path Type`          | Enum          | The base directory of the geometry files |
| `Vol Folder > Path`               | String        | The path of the folder containing the geometry files, relative to the `Vol Folder Path Type` |
| `Vol Video Texture > Path Type`   | Enum          | The base directory of the video texture |
| `Vol Video Texture > Path`        | String        | The path of the video texture file, relative to the `Vol Video Texture Path Type` |
| **Playback Settings**             |               |   |
| `Play On Start`                   | Bool          | Turn on if you want the vologram to play once the app/game starts |
| `Is Looping`                      | Bool          | Turn on if you want the vologram to play again after it finishes |
| `Audio On`\*                      | Bool          | Initialise an audio player to play the vologram's audio   |
| **Rendering Settings**            |               |   |
| `Material`\*                      | Material      | The Unity Material object used to render the vologram |
| `Texture Shader ID`               | String        | The Shader ID of the texture property that accepts the vologram texture |
| **Debugging Logging Options**     |               |   |
| `Enable Interface Logging`        | Enum          | Enables logging of native plugin-bridging code   | 
| `Enable Av Logging`               | Enum          | Enables logging of video-related native code  | 
| `Enable Geom Logging`             | Enum          | Enables logging of geometry-related native code  | 

**\* NOTES:** 
* Audio is still a work in progress on Android and so **audio is disabled for android builds**.
* When changing the Material in runtime, it is better to use the `ChangeMaterial` function (see below).

When the Unity Editor is in play mode, buttons will appear in the inspector that you can use to test different volograms 
* `Open/Close`: Open the files entered in the **Paths** fields, or close any open files
* `Play/Pause`: Play or pause the playback of the vologram
* `Restart`: Resets the vologram to the start of playback 

## Functions 

The following functions can be called through the `VolPlayer` component, e.g. `GetComponent<VolPlayer>().Open();`

| Function | Return Type | Description |
| --- | --- | --- |
| `Open()` | Bool | Attempts to open the files enter under **Paths**, returns True if successful, False otherwise |
| `Close()` | Bool | Closes the video file and frees the geometry data, returns True if successful, False otherwise |
| `Play()` | Void | Starts or resumes the vologram playback |
| `Pause()` | Void | Pauses the vologram playback |
| `Restart()` | Bool | Returns the vologram to the start of playback, if `Play On Start` is set to true then playback will begin after reset. Returns True is restart was successful, False otherwise | 
| `GetVideoWidth()` | Int | Returns the pixel width of the video texture, the video texture file must be open | 
| `GetVideoHeight()` | Int | Returns the pixel height of the video texture, the video texture file must be open | 
| `GetVideoFrameRate()` | Double | Returns the frame rate of the video texture, the video texture file must be open | 
| `GetVideoNumberOfFrames()` | Long | Returns the number of frame in the video texture, the video texture file must be open |
| `GetVideoDuration()` | Double | Returns the duration of the video texture in seconds, the video texture file must be open |
| `GetVideoFrameSize()` | Long | Returns the number of bytes in a frame of the video texture, the video texture file must be open |
| `SetMute(bool muted)` | Void | If audio is enabled, toggle the audio sound | 
| `ChangeMaterial(Material newMaterial)` | Void | Change the material used by the player |

## Properties 

The following properties can also be accessed from the `VolPlayer` component, e.g. `GetComponent<VolPlayer>().IsOpen;`

| Property | Type | Description |
| --- | --- | --- | 
| `IsOpen` | Bool | Returns True if a set of files is open, False otherwise |
| `IsPlaying` | Bool | Returns True if the vologram is playing, False otherwise |

## Notes on Materials and Shaders

You can use your own shaders and materials with the volograms player, include URP and HDRP materials. 
Ensure that your Shader ID matches that of the `Texture Shader ID` property, otherwise the volograms'
texture won't appear.

## iOS Unity Build
After importing the plugin into Unity and building your project for iOS, the Xcode build may fail due to `undefined symbols` error. This is because some of the frameworks that are required by the iOS plugin are not included in a Unity-Xcode project by default. Specifically, these frameworks/libs may be missing:

* `libbz2.tbd`
* `libz.tbd`
* `VideoToolbox.framework`

These can be added by going to the `Build Phases` of the Unity-Xcode project by:

* Selecting the `Unity-iPhone` project in the Navigator
* Selecting the `UnityFramework.framework` target
* Selecting `Build Phases` in the toolbar
* Expand `Link Binary With Libraries`

Then click the `+` button and searching for the above frameworks and libs by name

# Contact

For any issues relating to this plugin please contact [support@volograms.com](mailto:support@volograms.com)