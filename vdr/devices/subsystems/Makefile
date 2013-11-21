#
# Makefile for vdr utils
#
# $Id: Makefile 2.4 2012/12/18 13:35:54 kls Exp $

### The archiver options:

AR       ?= ar
ARFLAGS  ?= ru
RANLIB   ?= ranlib

-include ../../Make.config

### The object files (add further files here):

OBJS = DeviceAudioSubsystem.o \
       DeviceChannelSubsystem.o \
       DeviceCommonInterfaceSubsystem.o \
       DeviceImageGrabSubsystem.o \
       DevicePIDSubsystem.o \
       DevicePlayerSubsystem.o \
       DeviceReceiverSubsystem.o \
       DeviceSectionFilterSubsystem.o \
       DeviceSPUSubsystem.o \
       DeviceTrackSubsystem.o \
       DeviceVideoFormatSubsystem.o

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Targets:

all: subsystems.a

subsystems.a : $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.a *.so *.tgz core* *~
