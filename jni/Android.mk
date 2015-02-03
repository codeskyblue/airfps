#module inject
LOCAL_PATH		:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := myfps
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2
LOCAL_SRC_FILES := myfps.c

include $(BUILD_SHARED_LIBRARY)
