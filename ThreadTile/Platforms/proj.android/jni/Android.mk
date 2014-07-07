LOCAL_PATH := $(abspath $(call my-dir)/../../..)
HEADER_ROOT := $(LOCAL_PATH)
GYP_CONFIGURATION ?= Default

include $(LOCAL_PATH)/ThreadTile.mk

$(call import-module,CocosDenshion/android) \
$(call import-module,cocos2dx) \
$(call import-module,extensions)