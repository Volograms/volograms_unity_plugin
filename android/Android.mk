
$(info ffmpeg kit path $(FFMPEG_KIT))

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
LOCAL_MODULE 				:= avcodec
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/arm64-v8a/lib/libavcodec.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/armeabi-v7a/lib/libavcodec.so
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86/lib/libavcodec.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86_64/lib/libavcodec.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

# include $(CLEAR_VARS)
# LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
# LOCAL_MODULE 				:= avdevice
# ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/arm64-v8a/lib/libavdevice.so 
# endif
# ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/armeabi-v7a/lib/libavdevice.so 
# endif
# ifeq ($(TARGET_ARCH_ABI),x86)
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86/lib/libavdevice.so 
# endif
# ifeq ($(TARGET_ARCH_ABI),x86_64)
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86_64/lib/libavdevice.so 
# endif
# include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
LOCAL_MODULE 				:= avfilter
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/arm64-v8a/lib/libavfilter.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/armeabi-v7a/lib/libavfilter.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86/lib/libavfilter.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86_64/lib/libavfilter.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
LOCAL_MODULE 				:= avformat
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/arm64-v8a/lib/libavformat.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/armeabi-v7a/lib/libavformat.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86/lib/libavformat.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86_64/lib/libavformat.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
LOCAL_MODULE 				:= avutil
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/arm64-v8a/lib/libavutil.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/armeabi-v7a/lib/libavutil.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86/lib/libavutil.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86_64/lib/libavutil.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

# include $(CLEAR_VARS)
# LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
# LOCAL_MODULE				:= swresample
# ifeq ($(TARGET_ARCH_ABI),arm64-v8a) 
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/arm64-v8a/lib/libswresample.so 
# endif
# ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/armeabi-v7a/lib/libswresample.so 
# endif   
# ifeq ($(TARGET_ARCH_ABI),x86)
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86/lib/libswresample.so 
# endif
# ifeq ($(TARGET_ARCH_ABI),x86_64)
#     LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86_64/lib/libswresample.so 
# endif
# include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
LOCAL_MODULE 				:= swscale
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/arm64-v8a/lib/libswscale.so 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/armeabi-v7a/lib/libswscale.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86/lib/libswscale.so 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_SRC_FILES := $(FFMPEG_KIT)/x86_64/lib/libswscale.so 
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE  			:= arm
LOCAL_PATH      			:= $(ANDROID_NDK_HOME)
LOCAL_CFLAGS    			:= -DBASISD_SUPPORT_KTX2=0 #-DANDROID_DEBUG  #-Werror 
LOCAL_LDLIBS    			:= -llog
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/arm64-v8a/include 
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/armeabi-v7a/include 
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/x86/include 
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_C_INCLUDES := $(FFMPEG_KIT)/x86_64/include 
endif
LOCAL_C_INCLUDES			+= $(NDK_PROJECT_PATH)/../shared/src/ $(NDK_PROJECT_PATH)/../shared/thirdparty/basis_universal/transcoder/ $(NDK_PROJECT_PATH)/../shared/thirdparty/
LOCAL_SRC_FILES				:= $(NDK_PROJECT_PATH)/../shared/src/vol_interface.c $(NDK_PROJECT_PATH)/../shared/src/vol_geom.c $(NDK_PROJECT_PATH)/../shared/src/vol_av.c $(NDK_PROJECT_PATH)/../shared/src/vol_basis.cpp $(NDK_PROJECT_PATH)/../shared/thirdparty/basis_universal/transcoder/basisu_transcoder.cpp
LOCAL_MODULE     			:= volplayer
LOCAL_SHARED_LIBRARIES 		+= avcodec avfilter avformat avutil swscale # avdevice swresample

include $(BUILD_SHARED_LIBRARY)
