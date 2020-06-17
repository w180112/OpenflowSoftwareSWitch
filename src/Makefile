############################################################
# OpenFlow Software Switch source makefile
############################################################

######################################
# Set variable
######################################
LDLIBS += -L../lib/com_util -lcom_util
LDLIBS += -L../lib/libbridge -lbridge

APP = osw

# all source are stored in SRCS-y
SRCS-y := ofpd.c ofp_sock.c ofp_fsm.c ofp_codec.c ofp_dbg.c ofp_cmd.c dp_sock.c dp.c dp_codec.c dp_flow.c ofp_flow.c ofp_print.c dp_port_info.c ofp.c mailbox.c

# Build using pkg-config variables if possible
$(shell pkg-config --exists libdpdk)

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overridden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

INCLUDE += -I. -I../lib/com_util -I../lib/libbridge #-I$(RTE_SDK)/lib/librte_eal/common
CFLAGS += $(INCLUDE) -Wall -fPIC -g3 -std=c99 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wno-implicit-fallthrough

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)

include $(RTE_SDK)/mk/rte.app.mk
