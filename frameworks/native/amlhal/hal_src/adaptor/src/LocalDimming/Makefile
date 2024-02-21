################################################################################
#
#  adap_ld
#
################################################################################
LOCAL_PATH = $(shell pwd)
LDFLAGS += -Wl,--no-as-needed -lstdc++ -lpthread -ldl -lrt -L$(STAGING_DIR)/usr/lib
CFLAGS += -Wall -Werror -Wno-unknown-pragmas -Wno-format -Wno-format-security -Wno-error=unused-result \
          -O3 -fexceptions -fnon-call-exceptions -D_GNU_SOURCE -I$(STAGING_DIR)/usr/include


adap_pq_SRCS  = \
	$(LOCAL_PATH)/adap_ld.cpp \
	$(NULL)

adap_HEADERS = \
	-I$(LOCAL_PATH)/../../inc \
	-I$(LOCAL_PATH)/../../../lib/pq/LibTableLD \
	$(NULL)

# ---------------------------------------------------------------------
#  Build rules
BUILD_TARGETS = libadap_ld.so \

.PHONY: all install clean

libadap_ld.so: $(adap_pq_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -fPIC $(adap_HEADERS) \
	-L$(LOCAL_PATH)/../log -llog_cout \
	-L$(LOCAL_PATH)/../../../lib/pq/LibTableLD -l_PQTableLD \
	-o $@ $^ $(LDLIBS)

all: $(BUILD_TARGETS)

clean:
	rm -f *.o $(BUILD_TARGETS)

install:
	install -m 0644 libadap_ld.so $(TARGET_DIR)/usr/lib

uninstall:
	rm -f $(TARGET_DIR)/usr/lib/libadap_ld.so
