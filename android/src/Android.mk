FFMPEG_PATH 				:= /Users/patrick/Desktop/ThirdParty/ffmpeg-kit/android/libs/$(TARGET_ARCH_ABI)/
UNITY_INCLUDE 				:= ${UNITY_APP}/Contents/PluginAPI/

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avcodec
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libavcodec.so 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avdevice
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libavdevice.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avfilter
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libavfilter.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avformat
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libavformat.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avutil
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libavutil.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE				:= swresample
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libswresample.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= swscale
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libswscale.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= ffmpegkit
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libffmpegkit.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= ffmpegkit_abidetect
LOCAL_SRC_FILES 			:= $(FFMPEG_PATH)/libffmpegkit_abidetect.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE  			:= arm
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_CFLAGS    			:= -Werror -DANDROID_DEBUG
LOCAL_LDLIBS    			:= -llog
#LOCAL_C_INCLUDES 			+= $(UNITY_INCLUDE)/
LOCAL_C_INCLUDES 			+= /usr/local/Cellar/ffmpeg/4.4_2/include/
LOCAL_C_INCLUDES			+= $(NDK_PROJECT_PATH)/../../shared/src/
LOCAL_SRC_FILES				:= ../../shared/src/vol_interface.c ../../shared/src/vol_geom.c ../../shared/src/vol_av.c
LOCAL_MODULE     			:= volplayer
LOCAL_SHARED_LIBRARIES 		+= avcodec avdevice avfilter avformat avutil swresample swscale ffmpegkit ffmpegkit_abidetect

include $(BUILD_SHARED_LIBRARY)
