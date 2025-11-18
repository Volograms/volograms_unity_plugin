Volograms Unity SDK Release Notes 

Check the README in the for information on the VolPlayer component and FAQs

**v1.1.2** (18-11-25)
* Add support for streaming volograms in buffer mode: MacOS, iOS, and Android.
* Streaming mode selectable from Editor.

**v1.1.1** (18-11-25)
* Fix issue with looping when playOnStart is false.
* Fix issue with Restart() and Open() when playOnStart is false. It shows first frame but then stops.

**v1.1.0** (17-11-25)
* Add support for streaming volograms in buffer mode (Windows only).

**v1.0.2** (05-11-25)
* Add support for Mac OS.

**v1.0.1** (04-11-25)
* Add support for Android.

**v1.0.0** (31-10-25)
* Add support for the new single-vols file that includes texture and audio.
* New folder structure allowing for installation as a package. 
* Support for Win64 inside the package, other platforms will be added later.

**v0.1.2** (21-02-22)
* Changed the logging controls, giving users the ability to control which type of logging message are seen
* Fixed issue where logging messages were displayed with the wrong char set on Windows
* Added utility script that imports the volplayer libs with the correct settings

**v0.1.1** (16-02-22)
* Fixed issues with audio not pausing when vologram is paused
* Added ability to mute and unmute vologram in runtime 
* Added post-build script that adds required frameworks and libs to Xcode projects using the iOS player 

**v0.1.0** (14-02-22)
* Supported platforms: 
    * iOS
    * MacOS (excluding Macs with M1 chips)
    * Windows
    * Android (WIP) 
* Added functions to Play and Pause volograms
* Added functions to Open and Close vologram files
* Added function to Restart volograms
* Added function to play audio - due to performance issues playing audio on Android is temporarily disabled
* Added README with documentation on the VolPlayer component functions and inspector layout 
* Added custom editor inspector for VolPlayer component in Unity Editor 
* Added functions that enable debug messages from native code to print in Unity 
