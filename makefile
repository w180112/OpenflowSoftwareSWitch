############################################################
# ofpagent makefile
############################################################

######################################
# Set variable
######################################	
CC	= gcc -g
INCLUDE = -I./src/ -I./lib/com_util -I./lib/libbridge
CFLAGS = $(INCLUDE) -Wall -fPIC -g -std=c99 -D_XOPEN_SOURCE -D_DEFAULT_SOURCE

TARGET = osw
SRC = ./src/ofpd.c ./src/ofp_sock.c ./src/ofp_fsm.c ./src/ofp_codec.c ./src/ofp_dbg.c ./src/ofp_cmd.c ./src/dp_sock.c ./src/dp.c ./src/dp_codec.c ./src/dp_flow.c

OBJ = $(SRC:.c=.o)

SUBDIR = lib/libbridge lib/com_util

BUILDSUBDIR = $(SUBDIR:%=build-%)
CLEANSUBDIR = $(SUBDIR:%=clean-%)

all: $(BUILDSUBDIR) $(TARGET)

$(BUILDSUBDIR):
	${MAKE} -C $(@:build-%=%)

.PHONY: $(BUILDSUBDIR)

######################################
# Compile & Link
# 	Must use \tab key after new line
######################################
$(TARGET): $(OBJ) $(OBJECTS) ./src/*.h
	$(CC) $(CFLAGS) -L./lib/com_util -L./lib/libbridge $(OBJ) -o $(TARGET) \
	-static -lcom_util -lbridge -lpthread

######################################
# Clean 
######################################
clean: $(CLEANSUBDIR)

$(CLEANSUBDIR):
	$(MAKE) -C  $(@:clean-%=%) clean
	rm -f src/*.o osw

