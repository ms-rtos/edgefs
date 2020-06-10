#*********************************************************************************************************
#
#                                 北京翼辉信息技术有限公司
#
#                                   微型安全实时操作系统
#
#                                       MS-RTOS(TM)
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: Makefile
#
# 创   建   人: IoT Studio
#
# 文件创建日期: 2020 年 05 月 13 日
#
# 描        述: 本文件由 IoT Studio 生成，用于配置 Makefile 功能，请勿手动修改
#*********************************************************************************************************

#*********************************************************************************************************
# Include config.mk
#*********************************************************************************************************
CONFIG_MK_EXIST = $(shell if [ -f ../config.mk ]; then echo exist; else echo notexist; fi;)
ifeq ($(CONFIG_MK_EXIST), exist)
include ../config.mk
else
CONFIG_MK_EXIST = $(shell if [ -f config.mk ]; then echo exist; else echo notexist; fi;)
ifeq ($(CONFIG_MK_EXIST), exist)
include config.mk
else
CONFIG_MK_EXIST =
endif
endif

#*********************************************************************************************************
# Include MS-RTOS base config.mk
#*********************************************************************************************************
EMPTY =
SPACE = $(EMPTY) $(EMPTY)

MSRTOS_BASE_PATH_BAK := $(MSRTOS_BASE_PATH)
TOOLCHAIN_PREFIX_BAK := $(TOOLCHAIN_PREFIX)
DEBUG_LEVEL_BAK      := $(DEBUG_LEVEL)
CPU_TYPE_BAK         := $(CPU_TYPE)
FPU_TYPE_BAK         := $(FPU_TYPE)

MSRTOS_BASE_CONFIGMK = $(subst $(SPACE),\ ,$(MSRTOS_BASE_PATH))/config.mk
include $(MSRTOS_BASE_CONFIGMK)

MSRTOS_BASE_PATH := $(MSRTOS_BASE_PATH_BAK)
DEBUG_LEVEL      := $(DEBUG_LEVEL_BAK)

ifneq ($(TOOLCHAIN_PREFIX_BAK),)
TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREFIX_BAK)
endif

ifneq ($(CPU_TYPE_BAK),)
CPU_TYPE := $(CPU_TYPE_BAK)
endif

ifneq ($(FPU_TYPE_BAK),)
FPU_TYPE := $(FPU_TYPE_BAK)
endif

#*********************************************************************************************************
# Include header.mk
#*********************************************************************************************************
MKTEMP = $(subst $(SPACE),\ ,$(MSRTOS_BASE_PATH))/libmsrtos/src/mktemp

include $(MKTEMP)/header.mk

#*********************************************************************************************************
# Include targets makefiles
#*********************************************************************************************************
include libedgefs.mk

#*********************************************************************************************************
# Include end.mk
#*********************************************************************************************************
include $(END_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
