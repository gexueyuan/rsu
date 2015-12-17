ifeq ($(BUILD_CONFIG_PATH),)
$(error "You Must pass the BUILD_CONFIG_PATH to this Makefile")
endif
-include $(BUILD_CONFIG_PATH)/envsetup.mk

SRC_ROOT=../../

APP_MOD_NAME = 
APP_MOD_SCRIPT = 
APP_MODE_SOURCES =   \

SHARE_LIB_NAME = libsc.so
SHARE_LIB_SOURCES = \
					 $(wildcard cmn/*.c) \
					 $(wildcard internal/*.c) \
					 $(wildcard internal/adapt/tiger4/*.c) \
					 $(wildcard $(SRC_ROOT)/app/sdk-extend/xlat/*.c) \
					 $(wildcard $(SRC_ROOT)/app/sdk-extend/msghandler/*.c) \

APP_MOD_INCLDUES =  -I$(SRC_ROOT)/app/sc/cmn \
					-I$(SRC_ROOT)/app/sc/internal \
					-I$(SRC_ROOT)/drv/aal/include \
					-I$(SRC_ROOT)/drv/sdl/include \
					-I$(SRC_ROOT)/include \
					-I$(SRC_ROOT)/app/sdk-extend/inc \
					-I$(SRC_ROOT)/app/sdk-extend/linux \
					-I$(SRC_ROOT)/app/sdk-extend/xlat \
					-I$(SRC_ROOT)/plat/libs/include \
					-I$(SRC_ROOT)/plat/include \
					-I$(SRC_ROOT)/plat/include/osal \
					-I$(SRC_ROOT)/include \
					-I$(SRC_ROOT)/include/aal \
					-I$(SRC_ROOT)/include/sdl \
					-I$(SRC_ROOT)/plat/include/sdl \
					-I$(SRC_ROOT)/plat/event \

APP_MOD_FLAGS =  -DHAVE_EXT_MODE_SC
APP_MOD_LD_FLAGS = -L$(SRC_ROOT)/plat -lplat_user


OBJ_DIR = obj/

include $(BUILD_CONFIG_PATH)app-rules.mk
