LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
		HostPlay_new.cpp \


LOCAL_SHARED_LIBRARIES := \
	libui liblog libcutils libutils libbinder  \
        libgui libmedia \

LOCAL_MODULE:= libhostplay

LOCAL_C_INCLUDES := \
		$(call include-path-for, audio-utils) \
		$(TOP)/hardware/qcom/audio/hal

include $(BUILD_SHARED_LIBRARY)
