#UNITY_INCLUDE 				:= ${UNITY_APP}/Contents/PluginAPI/

$(info ffmpeg kit path $(FFMPEG_KIT))

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avcodec
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/lib/libavcodec.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/lib/libavcodec.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/lib/libavcodec.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/lib/libavcodec.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avdevice
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/lib/libavdevice.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/lib/libavdevice.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/lib/libavdevice.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/lib/libavdevice.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avfilter
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/lib/libavfilter.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/lib/libavfilter.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/lib/libavfilter.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/lib/libavfilter.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avformat
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/lib/libavformat.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/lib/libavformat.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/lib/libavformat.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/lib/libavformat.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= avutil
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/lib/libavutil.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/lib/libavutil.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/lib/libavutil.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/lib/libavutil.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE				:= swresample
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/lib/libswresample.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/lib/libswresample.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/lib/libswresample.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/lib/libswresample.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_MODULE 				:= swscale
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/lib/libswscale.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/lib/libswscale.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/lib/libswscale.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/lib/libswscale.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE  			:= arm
LOCAL_PATH      			:= $(NDK_PROJECT_PATH)
LOCAL_CFLAGS    			:= -Werror -DANDROID_DEBUG #-DENABLE_UNITY_RENDER_FUNCS
LOCAL_LDLIBS    			:= -llog
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/prebuilt/android-arm64/ffmpeg/include 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/prebuilt/android-arm/ffmpeg/include 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/prebuilt/android-x86/ffmpeg/include 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/prebuilt/android-x86_64/ffmpeg/include 
endif
#LOCAL_C_INCLUDES 			+= $(UNITY_INCLUDE)/
LOCAL_C_INCLUDES			+= $(NDK_PROJECT_PATH)/../shared/src/
LOCAL_SRC_FILES				:= ../shared/src/vol_interface.c ../shared/src/vol_geom.c ../shared/src/vol_av.c ../shared/src/vol_util.c
LOCAL_MODULE     			:= volplayer
LOCAL_SHARED_LIBRARIES 		+= avcodec avdevice avfilter avformat avutil swresample swscale

include $(BUILD_SHARED_LIBRARY)
