LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := Nen

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/NenEngine/include \
                    $(LOCAL_PATH)/NenEngine/include/Nen \
                    $(LOCAL_PATH)/NenEngine/libs/ImGui/include \
                    $(LOCAL_PATH)/NenEngine/libs/Lua/include \
                    $(LOCAL_PATH)/NenEngine/libs/mojoAL \
                    $(LOCAL_PATH)/NenEngine/libs/rapidjson/include \
                    $(LOCAL_PATH)/NenEngine/libs/sol2/include \

LOCAL_CFLAGS := -DSOL_NO_EXCEPTIONS -DNEN_NO_EXCEPTION -DMOBILE
SRC_PATH := $(LOCAL_PATH)/src/Nen \
            $(LOCAL_PATH)/NenEngine/libs/ImGui/source \
            $(LOCAL_PATH)/NenEngine/libs/Lua/source \
            $(LOCAL_PATH)/NenEngine/libs/mojoAL

LOCAL_SRC_FILES := \
    $(wildcard $(LOCAL_PATH)/NenEngine/src/nen/*/*.cpp) \
    $(wildcard $(LOCAL_PATH)/NenEngine/src/nen/Render/OpenGLES/*.cpp) \
    $(wildcard $(LOCAL_PATH)/NenEngine/libs/ImGui/source/gles/*.cpp) \
    $(wildcard $(LOCAL_PATH)/NenEngine/libs/Lua/source/*.c) \
    $(wildcard $(LOCAL_PATH)/NenEngine/libs/mojoAL/mojoal.c)
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_ttf SDL2_net
LOCAL_LDLIBS := -lGLESv3 -llog
LOCAL_EXPORT_C_INCLUDES += $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
