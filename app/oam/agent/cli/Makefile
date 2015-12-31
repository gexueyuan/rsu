include $(TOPDIR)/config.mk
export CLI_AGENT_DIR=$(shell pwd)


TARGET=cli_agent

CLISUBDIRS-y = main command common lib
CLISUBDIRS-y += $(TOPDIR)/app/oam/cmn \
    $(TOPDIR)/app/oam/external \
    $(TOPDIR)/app/oam/test \
    $(TOPDIR)/app/oam/test/external \


CLIOBJ_FILES =$(CLISUBDIRS-y:%=%/*.o)

CLI_LDFLAGS = -lrt -pthread -L$(LIBDIR)/readline/ -lreadline -ltermcap -L$(shell pwd)/common -Bstatic -ltermcap -lm 



.PHONEY: all clean app install

all: app install

app:
#	@echo $(DEVICEDIR)
#	@echo $(INCLUDEFLAGS)

	@for subdir in $(CLISUBDIRS-y); do \
		(cd $$subdir && $(MAKE)) || exit 1; \
	done

	$(CC) $(CLIOBJ_FILES) $(CLI_LDFLAGS) -o $(TARGET)
	$(STRIP) --strip-unneeded  --remove-section=.comment --remove-section=.note $(TARGET)

install:
	cp -f $(TARGET) $(IMAGES_DIR)/
	cp -f $(TARGET) $(TFTP_DIR)/

clean:
	-rm -rf $(TARGET)
	@for subdir in $(CLISUBDIRS-y); do \
		(cd $$subdir && $(MAKE) $@) || exit 1; \
	done
