CROSS_COMPILE ?=
CROSS_ROOT ?=
PKG_CONFIG ?= pkg-config

CFLAGS ?= $(shell $(PKG_CONFIG) --cflags libxml-2.0)
ifneq ($(CROSS_ROOT),)
CFLAGS += -I$(CROSS_ROOT)/include
endif
CFLAGS_EXTRA = -Wall -Wextra -Wno-unused-function -Wno-unused-parameter

LDFLAGS ?= $(shell $(PKG_CONFIG) --libs libxml-2.0)
LDFLAGS_EXTRA =
ifneq ($(CROSS_ROOT),)
LDFLAGS += -L$(CROSS_ROOT)/lib
endif
LDFLAGS_EXTRA +=

all:
	gcc ./cxq.c $(CFLAGS) $(LDFLAGS) $(LDFLAGS_EXTRA) -o cxq

.PHONY: clean
clean:
	rm cxq
