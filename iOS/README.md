# Building the Unity Native Plugin for iOS

Since Xcode projects can only be built on Apple device, this guide assumes the use of a computer running OSX.

## Project Layout 
* Open the `vol_unity_lib_ios.xcodeproj`
* The project hierarchy looks like this: 

![Project Hierarchy](/readme_resources/image-02.png)

* **IMPORTANT**: Take note that the `src` folder is actually the [`shared/src`](/shared/src/) folder, and all files contained in this folder are used by all platforms. Changing these files will affect other platforms' projects.

## Build Settings
* Access the **target** build settings
    * Click the `vol_unity_lib_ios` project in project navigator
    * Select `Build Settings` on the top toolbar 
    * On the left-hand side of the main pane, under `Targets`, select the `vol_unity_lib_ios` lib target 
  
![Build Settings](/readme_resources/image-06.png)

* Make sure the following settings have the following values:

| Build Settings Key                    | Build Setting Value |
| ------------------------------------- | ------------------- |
| `Architectures > Base SDK`            | `iOS`               |
| `Deployment > Target Device Families` | `iPhone, iPad`      |
| `Deployment > iOS Deploymeny Target`  | `iOS 11.0`          |

The following build settings have a list of values:

| Build Settings Key                    | Build Setting Values                            |
| ------------------------------------- | ----------------------------------------------- |
| `Search Paths > Header Search Paths`  | `"$(SRCROOT)/vol_unity_lib_ios/ffmpeg/include"` |
|                                       | `"$(SRCROOT)/../../shared/src"`                 |
| `Search Paths > Library Search Paths` | `$(inherited)`                                  |
|                                       | `$(PROJECT_DIR)/vol_unity_lib_ios/ffmpeg/lib`   |

All the entries for the `Search Paths` are **non-recursive**.

![Header Search Paths](/readme_resources/image-03.png)

![Library Search Paths](/readme_resources/image-00.png)


## Build Phases
* In the Target Settings the Build Phases look like this: 

![Build Phases](/readme_resources/image-01.png)

* Note the following:
    * The platform should be `iOS` for **all** files and libs
    * Additional libraries will have to be added manually to `Link Binary With Libraries` if they do not already exist in the project, in particular the `.tbd` and `.framework` files.
    * Add these by clicking `+` button at the bottom of the `Link Binary With Libraries` section, and then search for them in the search bar
    * The `ffmpeg` libs should be added when the `ffmpeg` folder was imported into the project, but if not, you can drag and drop the `.a` files from the project hierarchy into this section. 
    * Ensure that the `vol_****.c` files are included in the `Compile Sources` phase and similarly ensure the `vol_****.h` files are included in the `Copy Files` phase.
    * Header files will be copied to a separate include folder which must be added to the Unity project. 
* Additionally to the above build phases, a custom `Run Script` build phase has been added with the following shell script 

```shell
# Where Xcode puts the built libraries
PRODUCT_BASE_DIR="${BUILT_PRODUCTS_DIR}/.."
# Location of the iOS lib
IOS_PRODUCT="${PRODUCT_BASE_DIR}/Release-iphoneos/lib${TARGET_NAME}.a"
# Location of the Simulator lib
SIM_PRODUCT="${PRODUCT_BASE_DIR}/Release-iphonesimulator/lib${TARGET_NAME}.a"
# Where we want the final lib to go
DEST_DIR="${PROJECT_DIR}/../../UnityVol/Plugins/iOS"
# Log file indicating is something is missing
LOG_FILE="${DEST_DIR}/readme.txt"

echo "${LOG_FILE}"

if [[ -f $IOS_PRODUCT ]]; then 
    if [[ -f $SIM_PRODUCT ]]; then 
        # Remove log file, not needed
        rm -f -- $LOG_FILE
        # Combine the iOS and Simulator libraries into one, so it will work on both
        # actual devices and simulators 
        lipo -create $IOS_PRODUCT $SIM_PRODUCT -output "${PRODUCT_BASE_DIR}/libVolAv.a"
        # Copy universal lib into dest for easy transfer to Unity
        cp -r -f "${PRODUCT_BASE_DIR}/libVolAv.a" "${DEST_DIR}"
        # We also need to copy over the header files
        cp -r -f "${BUILT_PRODUCTS_DIR}/include" "${DEST_DIR}"
    else 
        # Print log file with some helpful text
        echo "Both iOS product and Simulator products are required." > $LOG_FILE
        echo "Simulator product does not exist. Build with simulator target." >> $LOG_FILE
        echo "In Xcode, in the top toolbar, set the build target to something" >> $LOG_FILE
        echo "like 'iPad (8th Generation)' under 'iOS Simulators', then build." >> $LOG_FILE
        open -e LOG_FILE
    fi
else 
    # Print log file with some helpful text
    echo "Both iOS product and Simulator products are required." > $LOG_FILE
    echo "iOS product does not exist. Build with iOS target." >> $LOG_FILE
    echo "In Xcode, in the top toolbar, set the build target to" >> $LOG_FILE
    echo "'Any iOS Device (arm64)' under 'Build', then build." >> $LOG_FILE
    open -e LOG_FILE
fi

# Open the dest folder in Finder
open $DEST_DIR
```

* This will ensure that libs for both iOS devices and iOS simulators are built - it then combines these into a universal lib that can be used for both release and debug 
* If everything is correct, then the files required by Unity are copied to the [`UnityVol` ](/UnityVol/) folder
* This means that you have to do **2 builds**:
    * Before building make sure the build is using the `Release` build configuration. 
    * Edit the target's scheme by clicking the build target in the top toolbar and selecting `Edit Scheme`
    ![Edit Scheme](/readme_resources/image-14.png)

    * Ensure the `Run` `Build Configuration` is set to `Release`.
    ![Build Configuration](/readme_resources/image-15.png)

    * The first build has a target of `Any iOS Device (arm64)`  
  
    ![Device Build](/readme_resources/image-04.png)

    * The second build has a target of a simulator (e.g. `iPad (8th Generation)`) 
  
    ![Simulator Build](/readme_resources/image-05.png)

* After this, the final lib file will be at [`UnityVol/Plugins/iOS/libVolAv.a`](/UnityVol/Plugins/iOS/libVolAv.a) with the corresponding header files located in [`UnityVol/Plugins/iOS/include`](/UnityVol/Plugins/iOS/include/).

 