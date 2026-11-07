#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#  *  http://www.apache.org/licenses/LICENSE-2.0
#  * Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

PWD:=$(shell pwd)
# Toolchain commands
#CROSS_COMPILE ?= /usr/lchome/yuzhihuang/Mstar/335/arm-buildroot-linux-uclibcgnueabihf-4.9.4/bin/arm-buildroot-linux-uclibcgnueabihf-
CROSS_COMPILE ?= /home/altobeam/rv1106_sdk/sololinker/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
# The path of LIB 
#NIMBLE_LDFLAGS ?= /usr/lchome/fuzhenzhou/ble_test/ATBM_WIFI_SVN3115_AsmLite18378_ARESM18330_Meru18648_20231106/ble_host/ble_smart_cfg_lib
NIMBLE_LDFLAGS ?= $(shell pwd)
CC      :=  $(CROSS_COMPILE)gcc
CXX     :=  $(CROSS_COMPILE)g++
LD      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
AS      := $(CROSS_COMPILE)as
NM      := $(CROSS_COMPILE)nm
OBJDUMP := $(CROSS_COMPILE)objdump
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE    := $(CROSS_COMPILE)size
BLEHOST = nimble_v42
MESH_OPEN = n
NIMBLE_ROOT = $(PWD)/../$(BLEHOST)

# Configure NimBLE variables
#NIMBLE_ROOT := ../../..
#NIMBLE_CFG_TINYCRYPT := 1

#include $(NIMBLE_ROOT)/Makefile_host.include
#include $(NIMBLE_ROOT)/porting/nimble/Makefile.defs

NIMBLE_INCLUDE_PATH := \
		-I$(NIMBLE_ROOT)/cli \
		-I$(NIMBLE_ROOT)/nimble/include \
		-I$(NIMBLE_ROOT)/nimble/host/services/ans/include \
		-I$(NIMBLE_ROOT)/nimble/host/services/bas/include  \
		-I$(NIMBLE_ROOT)/nimble/host/services/gap/include  \
		-I$(NIMBLE_ROOT)/nimble/host/services/gatt/include \
		-I$(NIMBLE_ROOT)/nimble/host/services/ias/include \
		-I$(NIMBLE_ROOT)/nimble/host/services/lls/include \
		-I$(NIMBLE_ROOT)/nimble/host/services/tps/include \
		-I$(NIMBLE_ROOT)/nimble/host/services/dis/include \
		-I$(NIMBLE_ROOT)/porting/nimble/include	\
		-I$(NIMBLE_ROOT)/nimble/host/services/include \
		-I$(NIMBLE_ROOT)/nimble/host/src/include \
		-I$(NIMBLE_ROOT)/nimble/host/src/include \
		-I$(NIMBLE_ROOT)/../os/linux_app/include \
		-I$(NIMBLE_ROOT)/nimble/host/util/include  \
		-I$(NIMBLE_ROOT)/nimble/host/src/include \
		-I$(NIMBLE_ROOT)/nimble/host/src 	\
		-I$(NIMBLE_ROOT)/nimble/include \
		-I$(NIMBLE_ROOT)/nimble/host/include  


# Source files for NPL OSAL
SRC += \
	ble_smart_cfg_lib.c  \
	ble_smart_demo_lib.c 	\
	ble_master_demo_lib.c	\
	peer_lib.c
#SRC = $(wildcard *.c)

# Add NPL and all NimBLE directories to include paths
INC = \
	$(NIMBLE_ROOT)/../include \
	$(NIMBLE_INCLUDE) \
	$(TINYCRYPT_INCLUDE) 

INCLUDES := $(addprefix -I, $(INC))
INCLUDES += -I$(TOPDIR)
INCLUDES +=  $(NIMBLE_INCLUDE_PATH)

SRC_C  = $(filter %.c,  $(SRC))
SRC_CC = $(filter %.cc, $(SRC))

OBJ := $(SRC_C:.c=.o)
#OBJ += $(SRC_CC:.cc=.o)

#OBJS = $(patsubst %.c, %.o, $(SRC))

#TINYCRYPT_OBJ := $(TINYCRYPT_SRC:.c=.o)

CFLAGS =                    \
    $(NIMBLE_CFLAGS)        \
    $(INCLUDES)             \
    -g                      \
    -D_GNU_SOURCE       \
    $(NULL)

LIBS := -L $(PWD) -lrt -lpthread -lstdc++ -lnimble_linux

.PHONY: all clean
.DEFAULT: all

all: app_demo

clean:
	rm $(OBJ) -f
	rm app_demo -f

#$(TINYCRYPT_OBJ): CFLAGS+=$(TINYCRYPT_CFLAGS)

%.o: %.c
	$(CC)  -c $(INCLUDES) $(CFLAGS)  -o $@ $<

%.o: %.cc
	$(CXX)  -c $(INCLUDES) $(CFLAGS) -o $@ $<

app_demo: $(OBJ) 
	$(LD) -o $@ $^ $(LIBS)
	$(SIZE) $@
	$(CROSS_COMPILE)strip app_demo --strip-unneeded
	cp -rf app_demo $(PWD)/../../driver_install
