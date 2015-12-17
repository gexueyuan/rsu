include ./config.mk

export TOPDIR = $(shell pwd)
export LIBDIR=$(TOPDIR)/library
export IMAGES_DIR=$(TOPDIR)/images
export TFTP_DIR=/home/gexueyuan/tftp

TOOLS_DIR=$(TOPDIR)/tools

NOW = `date +"%Y%m%d"`
#DEVICENAME = `echo $(DEVICEDIR)|tr a-z A-Z`
DEVICENAME = CARSMART_RSU
MAJOR_VER = `grep MAJOR $(TOPDIR)/include/version.h | awk '{print $$3}'  | sed -e 's/(//'  -e 's/)//'`
MINOR_VER = `grep MINOR $(TOPDIR)/include/version.h | awk '{print $$3}'  | sed -e 's/(//'  -e 's/)//'`
BUILD_VER = `grep BUILD $(TOPDIR)/include/version.h | awk '{print $$3}'  | sed -e 's/(//'  -e 's/)//'`
VER=$(MAJOR_VER).$(MINOR_VER).$(BUILD_VER)
IMGNAME=$(DEVICENAME)_$(VER)_$(NOW)

.PHONEY: all clean image install 
all:lib app image

lib:
	@(cd $(TOPDIR)/osal/linux ; $(MAKE) ) || exit 1
	@(cd $(TOPDIR)/osal ; $(MAKE) ) || exit 1
	
app:lib
	@(cd $(TOPDIR)/app ; $(MAKE) ) || exit 1
	@(cd $(TOPDIR)/app/oam/agent/cli ; $(MAKE) ) || exit 1

image: 
	@(cd $(TOPDIR)/app ; $(MAKE) install) || exit 1
	@(cd $(TOPDIR)/app/oam/agent/cli ; $(MAKE) install) || exit 1

clean:
	@(cd $(TOPDIR)/osal/linux ; $(MAKE) clean ) || exit 1
	@(cd $(TOPDIR)/osal ; $(MAKE) clean ) || exit 1
	@(cd $(TOPDIR)/app ; $(MAKE) clean) || exit 1
	@(cd $(TOPDIR)/app/oam/agent/cli ; $(MAKE) clean) || exit 1
	#@rm -rf $(IMAGES_DIR)

