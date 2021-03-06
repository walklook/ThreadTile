# This file is generated by gyp; do not edit.

include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE := lib_ThreadTile
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
gyp_intermediate_dir := $(call local-intermediates-dir)
gyp_shared_intermediate_dir := $(call intermediates-dir-for,GYP,shared)

# Make sure our deps are built first.
GYP_TARGET_DEPENDENCIES :=

GYP_GENERATED_OUTPUTS :=

# Make sure our deps and generated files are built first.
LOCAL_ADDITIONAL_DEPENDENCIES := $(GYP_TARGET_DEPENDENCIES) $(GYP_GENERATED_OUTPUTS)

LOCAL_GENERATED_SOURCES :=

GYP_COPIED_SOURCE_ORIGIN_DIRS :=

LOCAL_SRC_FILES := \
	Classes/Map/TileLayer.cpp \
	Classes/AppDelegate.cpp \
	Classes/HelloWorldScene.cpp \
	Platforms/proj.android/jni/main.cpp


# Flags passed to both C and C++ files.
MY_CFLAGS_Default :=

MY_DEFS_Default :=


# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES_Default := \
	$(HEADER_ROOT)/Classes/Map \
	$(HEADER_ROOT)/Classes \
	$(COCOS2DX_ROOT)/cocos2dx \
	$(COCOS2DX_ROOT)/cocos2dx/include \
	$(COCOS2DX_ROOT)/cocos2dx/platform \
	$(COCOS2DX_ROOT)/cocos2dx/kazmath/include \
	$(COCOS2DX_ROOT)/CocosDenshion/include \
	$(COCOS2DX_ROOT)/extensions \
	$(COCOS2DX_ROOT)/extensions/CCBReader \
	$(COCOS2DX_ROOT)/extensions/CCArmature/external_tool/Json \
	$(COCOS2DX_ROOT)/extensions/CCArmature/external_tool/Json/lib_json \
	$(COCOS2DX_ROOT)/extensions/GUI/CCControlExtension \
	$(COCOS2DX_ROOT)/extensions/GUI/CCScrollView


# Flags passed to only C++ (and not C) files.
LOCAL_CPPFLAGS_Default :=


LOCAL_CFLAGS := $(MY_CFLAGS_$(GYP_CONFIGURATION)) $(MY_DEFS_$(GYP_CONFIGURATION))
LOCAL_C_INCLUDES := $(GYP_COPIED_SOURCE_ORIGIN_DIRS) $(LOCAL_C_INCLUDES_$(GYP_CONFIGURATION))
LOCAL_CPPFLAGS := $(LOCAL_CPPFLAGS_$(GYP_CONFIGURATION))
### Rules for final target.

LOCAL_LDFLAGS_Default :=


LOCAL_LDFLAGS := $(LOCAL_LDFLAGS_$(GYP_CONFIGURATION))

LOCAL_LDLIBS := \
	-llog \
	-landroid \
	-ldl


LOCAL_WHOLE_STATIC_LIBRARIES := \
	libcocosdenshion_static \
	libcocos_extension_static \
	libcocos2dx_static \
	libcocos_libpng_static \
	libcocos_jpeg_static \
	libcocos_libxml2_static \
	libcocos_libtiff_static \
	libcocos_libwebp_static \
	libcocos_curl_static \
	libwebsockets_static

# Enable grouping to fix circular references
LOCAL_GROUP_STATIC_LIBRARIES := true

# Add target alias to "gyp_all_modules" target.
.PHONY: gyp_all_modules
gyp_all_modules: lib_ThreadTile

# Alias gyp target name.
.PHONY: ThreadTile
Fables: lib_ThreadTile

LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
