#
#makefile for build atbm_wifi.ko
#
###############################################################################
#
# when release to customer ,the CUSTOMER_SUPPORT_USED must set to y!!!!!!!!!!!!!
#
###############################################################################
CUSTOMER_SUPPORT_USED=y
###############################################################################
#PLATFORM_MTK_9255 22
###############################################################################

export
platform ?= PLATFORM_DEFAULT
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=22
#Android
#Linux
sys ?= Android
#arch:arm or arm64 or mips(NVT98517)
ARCH ?= arm
#export 
#ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=$(platform)

ifeq ($(CUSTOMER_SUPPORT_USED),y)
    MAKEFILE_SUB ?= Makefile.build.customer
else
    MAKEFILE_SUB ?= Makefile.build
endif

ifeq ($(platform),)
    platform ?= PLATFORM_DEFAULT
endif

ifeq ($(KERNELRELEASE),)
    export DRIVER_PATH ?= $(shell pwd)

MODULE_NAME = atbm603x_comb_wifi_usb

########################################PLATFORM_DEFAULT###########################
    ifeq ($(platform),PLATFORM_DEFAULT)
        KERDIR ?=
        CROSS_COMPILE ?=
        export
        ARCH ?= 
        ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=24
    endif
###################################### END ########################################

    ifeq ($(platform),PLATFORM_MTK_9255)
        ifeq ($(sys),Linux)
            #KERDIR:=/usr/src/linux-5.4.234
	    KERDIR:=/usr/src/linux-headers-5.19.0-35-generic/
            CROSS_COMPILE:=
        else
            KERDIR:=/usr/lchome/zhengzhiqiang/mtk9255/git_9255/kernel/fusion/4.9
            CROSS_COMPILE:=/usr/lchome/zhengzhiqiang/mtk9255/git_9255/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
        endif
        export
        arch = x86_64
ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=22
endif

#Makefile_build_customer
############################################################################
#			ATBM WiFi Product Select
#CONFIG_ATBM601x: 1T1R 80211b/g/n, HT20
#if yout want to change .conf ,please do make menuconfig in current path
############################################################################
-include .config
############################################################################
#
#	The Follow Code Of The Makefile Should Not Be Changed 
#
############################################################################
PWD:=$(shell pwd)
WIFI_INSTALL_DIR := $(PWD)/driver_install

export WIFI_PROJ_DIR=$(PWD)
#####################################################

ifeq ($(CONFIG_ATBM_MODULE_DRIVER_NAME),)
CONFIG_ATBM_MODULE_DRIVER_NAME = "atbm_wlan"
endif
MODULES_NAME   ?= $(shell echo $(CONFIG_ATBM_MODULE_NAME))

################## BLE ###############################
ifeq ($(CONFIG_ATBM_BLE_HOST_DRIVER),y)
export CONFIG_ATBM_BLE_HOST_DRIVER=m
endif

KDIR=$(KERNEL_SRC)
M=$(shell pwd)

all:modules

modules:
	@echo "make PLATFORM_CROSS=$(platform)"
	@echo "make ARCH=$(ARCH)"
	@echo "make OUT_DIR=$(OUT_DIR)"
	rm -vrf $(OUT_DIR)/$(M)
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) SYS=$(sys) PLAT=$(platform) modules

modules_install:
	$(MAKE) INSTALL_MOD_STRIP=1 -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name $(MODULE_NAME).ko -exec cp {} ${OUT_DIR}/../vendor_lib/modules/ \;

clean:hal_clean
	rm -rf hal_apollo/*.o
	rm -rf hal_apollo/*.ko  
	rm -rf modules.* Module.* 


hal_clean:
	rm -rf hal_apollo/*.ko
	rm -rf hal_apollo/*.o
	rm -rf hal_apollo/*.mod.c
	rm -rf hal_apollo/*.cmd
	rm -rf hal_apollo/.*.cmd
	rm -rf hal_apollo/mac80211/*.cmd
	rm -rf hal_apollo/mac80211/.*.cmd
	rm -rf hal_apollo/mac80211/*.o
	rm -rf driver_install/*.ko
	find ./ -name ".*.cmd" -exec rm -f {} \;
	find ./ -name "*.o" -exec rm -f {} \;
	find ./ -name "*.ko" -exec rm -f {} \;
	find ./ -name "*.cmd" -exec rm -f {} \;
	find ./ -name "*.mod" -exec rm -f {} \;
	find ./ -name "*.order" -exec rm -f {} \;

strip:	
	$(CROSS_COMPILE)strip $(WIFI_INSTALL_DIR)/$(MODULES_NAME).ko --strip-unneeded

get_ver:
	@echo "**************************************"
	@echo "driver version"
	@cat hal_apollo/svn_version.h | awk '{print $3}'
	@echo "**************************************"

buid_config:
	$(MAKE) -C atbm_kconf clean
	$(MAKE) -C atbm_kconf mconf -f Makefile

menuconfig:buid_config
	@./atbm_kconf/mconf ./atbm_kconf/Kconfig

debug:
	$(MAKE) all ARCH=$(arch)  CROSS_COMPILE=$(CROSS_COMPILE) KDIR=$(KDIR) SYS=$(sys) PLAT=$(platform) DEBUG_ZEZER=y -j8


else
    ifeq ($(platform),PLATFORM_DEFAULT)
        export 
        ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=0
    endif
    ifeq ($(platform),PLATFORM_MTK_9255)
        export 
        ATBM_WIFI__EXT_CCFLAGS = -DATBM_WIFI_PLATFORM=22
    endif
    export 
include $(src)/Makefile.build.kernel
endif

