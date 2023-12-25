#################################################################################
#
# lib_PQTableLD
#
#################################################################################
-include curFile

LOCAL_PATH = $(shell pwd)
LDFLAGS += -Wl,--no-as-needed -lstdc++ -lpthread -ldl -lrt -L$(STAGING_DIR)/usr/lib
CFLAGS += -Wall -Werror -Wno-unknown-pragmas -Wno-format -Wno-format-security -Wno-error=unused-result \
          -O3 -fexceptions -fnon-call-exceptions -D_GNU_SOURCE -I$(STAGING_DIR)/usr/include

SRCS  = $(LOCAL_PATH)/PQTableLD.cpp
SRCS += $(LOCAL_PATH)/PQTableLDLoader.cpp
SRCS += $(LOCAL_PATH)/AML_LD_Table_Public.cpp

GENSRC = ./GenerateBinFileLD.cpp
GENSRC += ./PQTableLDLoader.cpp

BUILD_TARGETS = lib_PQTableLD.so \

.PHONY: all install clean

lib_PQTableLD.so: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -fPIC $(INC_SRCS) \
	-o $@ $^ $(LDLIBS)

all: $(BUILD_TARGETS)

clean:
	rm -f *.o $(BUILD_TARGETS)

GENclean:
	rm -f *.o

install:
	install -m 0644 lib_PQTableLD.so $(TARGET_DIR)/usr/lib

uninstall:
	rm -f $(TARGET_DIR)/usr/lib/lib_PQTableLD.so

FileLD: GENclean
	g++ -g -Wall -ansi -o GenLDFile $(GENSRC) $(CUR_PQFILE)
	@echo ===== GenLDFile LD BIN generated !! =====