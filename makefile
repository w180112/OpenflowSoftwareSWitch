############################################################
# OpenFlow Software Switch makefile
############################################################

######################################
# Set variable
######################################
SUBDIR = lib/libbridge lib/com_util src/

BUILDSUBDIR = $(SUBDIR:%=build-%)
CLEANSUBDIR = $(SUBDIR:%=clean-%)

all: $(BUILDSUBDIR)

$(BUILDSUBDIR):
	${MAKE} -C $(@:build-%=%)

.PHONY: $(BUILDSUBDIR)

######################################
# Clean 
######################################
clean: $(CLEANSUBDIR)

$(CLEANSUBDIR):
	$(MAKE) -C  $(@:clean-%=%) clean
	#rm -f src/*.o osw
