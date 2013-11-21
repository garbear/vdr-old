#
#      Copyright (C) 2013 Garrett Brown
#      Copyright (C) 2013 Lars Op den Kamp
#      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this Program; see the file COPYING. If not, see
#  <http://www.gnu.org/licenses/>.
#

.DELETE_ON_ERROR:

# Compiler flags:

PKGCONFIG ?= pkg-config

CC       ?= gcc
CFLAGS   ?= -g -O3 -Wall

CXX      ?= g++
CXXFLAGS ?= -g -O3 -Wall -Wextra -Werror=overloaded-virtual -Wno-parentheses -pthread

CDEFINES  = -D_GNU_SOURCE
CDEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

ifdef ANDROID
#LIBS      = -ljpeg -ldl -lintl $(shell $(PKGCONFIG) --libs freetype2 fontconfig)
LIBS      =
else
#LIBS      = -ljpeg -lpthread -ldl -lcap -lrt $(shell $(PKGCONFIG) --libs freetype2 fontconfig)
LIBS      = -lpthread
endif
#INCLUDES ?= $(shell $(PKGCONFIG) --cflags freetype2 fontconfig)
INCLUDES ?=

# Directories:

CWD             ?= $(shell pwd)
GTEST_DIR       ?= $(CWD)/lib/gtest
LSIDIR          ?= $(CWD)/lib/libsi
VDRDIR          ?= $(CWD)/vdr
VDRTESTDIR      ?= $(VDRDIR)/test
DEVICESDIR      ?= $(VDRDIR)/devices
DEVICESSUBYSDIR ?= $(VDRDIR)/devices/subsystems
FSDIR           ?= $(VDRDIR)/filesystem
SETTINGSDIR     ?= $(VDRDIR)/settings
UTILSDIR        ?= $(VDRDIR)/utils

DESTDIR   ?=

BINDIR    ?= /usr/local/bin
INCDIR    ?= /usr/local/include
MANDIR    ?= /usr/local/share/man
PCDIR     ?= /usr/local/lib/pkgconfig

INCLUDES += -I$(CWD) \
            -I$(VDRDIR)# \
#            -I$(GTEST_DIR)

# Flags passed to the preprocessor
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers

CPPFLAGS += -isystem $(GTEST_DIR)/include

# Object files

SILIB            = $(LSIDIR)/libsi.a
VDRLIB           = $(VDRDIR)/vdr.a
VDRTESTLIB       = $(VDRTESTDIR)/vdr_test.a
DEVICESLIB       = $(DEVICESDIR)/devices.a
DEVICESSUBSYSLIB = $(DEVICESSUBYSDIR)/subsystems.a
FSLIB            = $(FSDIR)/filesystem.a
SETTINGSLIB      = $(SETTINGSDIR)/settings.a
UTILSLIB         = $(UTILSDIR)/utils.a

#VDRLIBS = $(VDRLIB) \
#          $(SILIB) \
#          $(DEVICESLIB) \
#          $(DEVICESSUBSYSLIB) \
#          $(FSLIB) \
#          $(SETTINGSLIB) \
#          $(UTILSLIB)

VDRLIBS = $(VDRLIB)

VDRTESTLIBS = $(VDRTESTLIB)

#COBJS = android_sort.o android_strchrnul.o android_getline.o android_timegm.o

#CXXOBJS = audio.o channels.o ci.o config.o cutter.o device.o diseqc.o dvbdevice.o dvbci.o\
#       dvbplayer.o dvbspu.o dvbsubtitle.o eit.o eitscan.o epg.o filter.o font.o i18n.o interface.o keys.o\
#       lirc.o menu.o menuitems.o nit.o osdbase.o osd.o pat.o player.o plugin.o\
#       receiver.o recorder.o recording.o remote.o remux.o ringbuffer.o sdt.o sections.o shutdown.o\
#       skinclassic.o skinlcars.o skins.o skinsttng.o sourceparams.o sources.o spu.o status.o svdrp.o themes.o thread.o\
#       timers.o tools.o transfer.o vdr.o videodir.o

# Source documentation

DOXYGEN  ?= /usr/bin/doxygen
DOXYFILE  = Doxyfile

# User configuration

#-include Make.config

# Mandatory compiler flags:

#CFLAGS   += -fPIC
#CXXFLAGS += -fPIC

# Common include files:

ifdef DVBDIR
CINCLUDES += -I$(DVBDIR)
endif

DEFINES  += $(CDEFINES)
INCLUDES += $(CINCLUDES)

#ifdef HDRDIR
#HDRDIR   := -I$(HDRDIR)
#endif
#ifndef NO_KBD
#DEFINES += -DREMOTE_KBD
#endif
#ifdef REMOTE
#DEFINES += -DREMOTE_$(REMOTE)
#endif
#ifdef VDR_USER
#DEFINES += -DVDR_USER=\"$(VDR_USER)\"
#endif
#ifdef BIDI
#INCLUDES += $(shell pkg-config --cflags fribidi)
#DEFINES += -DBIDI
#LIBS += $(shell pkg-config --libs fribidi)
#endif

# The version numbers of VDR and the plugin API (taken from VDR's "config.h"):

VDRVERSION = $(shell sed -ne '/define VDRVERSION/s/^.*"\(.*\)".*$$/\1/p' config.h)
APIVERSION = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' config.h)

all: vdr.bin libvdr.so test #i18n plugins

test: test-vdr.bin

# Implicit rules:

#%.o: %.c
#	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

# Dependencies:

#MAKEDEP = $(CXX) -MM -MG
#DEPFILE = .dependencies
#$(DEPFILE): Makefile
#	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(COBJS:%.o=%.c) $(CXXOBJS:%.o=%.cpp) > $@

#-include $(DEPFILE)

# The main program:

vdr.bin: $(VDRDIR)/main.o $(VDRLIBS)
	$(CXX) $(CXXFLAGS) -rdynamic $(LDFLAGS) $^ $(LIBS) -o $@

addon.a: $(VDRDIR)/addon.o
	$(AR) $(ARFLAGS) $@ $(VDRDIR)/addon.o

libvdr.so: addon.a $(VDRLIBS)
	$(CXX) $(CXXFLAGS) -shared $(LDFLAGS) $^ $(LIBS) -o $@

test-vdr.bin: $(VDRLIBS) gtest_main.a $(VDRTESTLIBS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Wl,--whole-archive $(VDRTESTLIBS) -Wl,--no-whole-archive $(VDRLIBS) gtest_main.a  $(LIBS) -o $@

# The libsi library:

#$(SILIB):
#	$(MAKE) --no-print-directory -C $(LSIDIR) CXXFLAGS="$(CXXFLAGS)" DEFINES="$(CDEFINES)" all

# Builds gtest.a and gtest_main.a.

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h # $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

.PHONY: $(VDRLIBS) $(VDRTESTLIBS)
$(VDRLIB):
	$(MAKE) --no-print-directory -C $(VDRDIR) CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)" DEFINES="$(CDEFINES)" INCLUDES="$(INCLUDES)" all

$(VDRTESTLIB):
	$(MAKE) --no-print-directory -C $(VDRTESTDIR) CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)" DEFINES="$(CDEFINES)" INCLUDES="$(INCLUDES)" all

#$(FSLIB):
#	$(MAKE) --no-print-directory -C $(FSDIR) CXXFLAGS="$(CXXFLAGS)" DEFINES="$(CDEFINES)" all

#$(SETTINGSLIB):
#	$(MAKE) --no-print-directory -C $(SETTINGSDIR) CXXFLAGS="$(CXXFLAGS)" DEFINES="$(CDEFINES)" all

#$(UTILSLIB):
#	$(MAKE) --no-print-directory -C $(UTILSDIR) CXXFLAGS="$(CXXFLAGS)" DEFINES="$(CDEFINES)" all

# pkg-config file:

.PHONY: vdr.pc
vdr.pc:
	@echo "bindir=$(BINDIR)" > $@
	@echo "mandir=$(MANDIR)" >> $@
	@echo "configdir=$(CONFDIR)" >> $@
	@echo "videodir=$(VIDEODIR)" >> $@
	@echo "cachedir=$(CACHEDIR)" >> $@
	@echo "resdir=$(RESDIR)" >> $@
	@echo "libdir=$(LIBDIR)" >> $@
	@echo "locdir=$(LOCDIR)" >> $@
	@echo "plgcfg=$(PLGCFG)" >> $@
	@echo "apiversion=$(APIVERSION)" >> $@
	@echo "cflags=$(CFLAGS) $(CDEFINES) $(CINCLUDES) $(HDRDIR)" >> $@
	@echo "cxxflags=$(CXXFLAGS) $(CDEFINES) $(CINCLUDES) $(HDRDIR)" >> $@
	@echo "" >> $@
	@echo "Name: VDR" >> $@
	@echo "Description: Video Disk Recorder" >> $@
	@echo "URL: http://www.tvdr.de/" >> $@
	@echo "Version: $(VDRVERSION)" >> $@
	@echo "Cflags: \$${cflags}" >> $@

# Internationalization (I18N):

PODIR     = po
LOCALEDIR = locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr.mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/vdr.pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c) $(wildcard *.cpp)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=VDR --package-version=$(VDRVERSION) --msgid-bugs-address='<vdr-bugs@tvdr.de>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr.mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmsgs)

install-i18n: i18n
	@mkdir -p $(DESTDIR)$(LOCDIR)
	cp -r $(LOCALEDIR)/* $(DESTDIR)$(LOCDIR)

# The 'include' directory (for plugins):

include-dir:
	@mkdir -p include/vdr
	@(cd include/vdr; for i in ../../*.h; do ln -fs $$i .; done)
	@mkdir -p include/libsi
	@(cd include/libsi; for i in ../../libsi/*.h; do ln -fs $$i .; done)

# Plugins:

plugins: include-dir vdr.pc
	@failed="";\
	noapiv="";\
	oldmakefile="";\
	for i in `ls $(PLUGINDIR)/src | grep -v '[^a-z0-9]'`; do\
	    echo; echo "*** Plugin $$i:";\
	    # No APIVERSION: Skip\
	    if ! grep -q "\$$(LIBDIR)/.*\$$(APIVERSION)" "$(PLUGINDIR)/src/$$i/Makefile" ; then\
	       echo "ERROR: plugin $$i doesn't honor APIVERSION - not compiled!";\
	       noapiv="$$noapiv $$i";\
	       continue;\
	       fi;\
	    # Old Makefile\
	    if ! grep -q "PKGCFG" "$(PLUGINDIR)/src/$$i/Makefile" ; then\
	       echo "WARNING: plugin $$i is using an old Makefile!";\
	       oldmakefile="$$oldmakefile $$i";\
	       $(MAKE) --no-print-directory -C "$(PLUGINDIR)/src/$$i" CFLAGS="$(CFLAGS) $(CDEFINES) $(CINCLUDES)" CXXFLAGS="$(CXXFLAGS) $(CDEFINES) $(CINCLUDES)" LIBDIR="$(PLUGINDIR)/lib" VDRDIR="$(CWD)" all || failed="$$failed $$i";\
	       continue;\
	       fi;\
	    # New Makefile\
	    INCLUDES="-I$(CWD)/include"\
	    $(MAKE) --no-print-directory -C "$(PLUGINDIR)/src/$$i" VDRDIR="$(CWD)" || failed="$$failed $$i";\
	    if [ -n "$(LCLBLD)" ] ; then\
	       (cd $(PLUGINDIR)/src/$$i; for l in `find -name 'libvdr-*.so' -o -name 'lib$$i-*.so'`; do install $$l $(LIBDIR)/`basename $$l`.$(APIVERSION); done);\
	       if [ -d $(PLUGINDIR)/src/$$i/po ]; then\
	          for l in `ls $(PLUGINDIR)/src/$$i/po/*.mo`; do\
	              install -D -m644 $$l $(LOCDIR)/`basename $$l | cut -d. -f1`/LC_MESSAGES/vdr-$$i.mo;\
	              done;\
	          fi;\
	       fi;\
	    done;\
	# Conclusion\
	if [ -n "$$noapiv" ] ; then echo; echo "*** plugins without APIVERSION:$$noapiv"; echo; fi;\
	if [ -n "$$oldmakefile" ] ; then\
	   echo; echo "*** plugins with old Makefile:$$oldmakefile"; echo;\
	   echo "**********************************************************************";\
	   echo "*** While this currently still works, it is strongly recommended";\
	   echo "*** that you convert old Makefiles to the new style used since";\
	   echo "*** VDR version 1.7.36. Support for old style Makefiles may be dropped";\
	   echo "*** in future versions of VDR.";\
	   echo "**********************************************************************";\
	   fi;\
	if [ -n "$$failed" ] ; then echo; echo "*** failed plugins:$$failed"; echo; exit 1; fi

clean-plugins:
	@for i in `ls $(PLUGINDIR)/src | grep -v '[^a-z0-9]'`; do $(MAKE) --no-print-directory -C "$(PLUGINDIR)/src/$$i" clean; done
	@-rm -f $(PLUGINDIR)/lib/lib*-*.so.$(APIVERSION)

# Install the files (note that 'install-pc' must be first!):

install: install-pc install-bin install-dirs install-conf install-doc install-plugins install-i18n install-includes

# VDR binary:

install-bin: vdr.bin
	@mkdir -p $(DESTDIR)$(BINDIR)
	@cp --remove-destination vdr.bin svdrpsend $(DESTDIR)$(BINDIR)

# Configuration files:

install-dirs:
	@mkdir -p $(DESTDIR)$(VIDEODIR)
	@mkdir -p $(DESTDIR)$(CONFDIR)
	@mkdir -p $(DESTDIR)$(CACHEDIR)
	@mkdir -p $(DESTDIR)$(RESDIR)

install-conf:
	@cp -pn *.conf $(DESTDIR)$(CONFDIR)

# Documentation:

install-doc:
	@mkdir -p $(DESTDIR)$(MANDIR)/man1
	@mkdir -p $(DESTDIR)$(MANDIR)/man5
	@gzip -c vdr.1 > $(DESTDIR)$(MANDIR)/man1/vdr.1.gz
	@gzip -c vdr.5 > $(DESTDIR)$(MANDIR)/man5/vdr.5.gz
	@gzip -c svdrpsend.1 > $(DESTDIR)$(MANDIR)/man1/svdrpsend.1.gz

# Plugins:

install-plugins: plugins
	@-for i in `ls $(PLUGINDIR)/src | grep -v '[^a-z0-9]'`; do\
	      $(MAKE) --no-print-directory -C "$(PLUGINDIR)/src/$$i" VDRDIR=$(CWD) DESTDIR=$(DESTDIR) install;\
	      done
	@if [ -d $(PLUGINDIR)/lib ] ; then\
	    for i in `find $(PLUGINDIR)/lib -name 'lib*-*.so.$(APIVERSION)'`; do\
	        install -D $$i $(DESTDIR)$(LIBDIR);\
	        done;\
	    fi

# Includes:

install-includes: include-dir
	@mkdir -p $(DESTDIR)$(INCDIR)
	@cp -pLR include/vdr include/libsi $(DESTDIR)$(INCDIR)

# pkg-config file:

install-pc: vdr.pc
	if [ -n "$(PCDIR)" ] ; then\
	   mkdir -p $(DESTDIR)$(PCDIR) ;\
	   cp vdr.pc $(DESTDIR)$(PCDIR) ;\
	   fi

# Source documentation:

srcdoc:
	@cat $(DOXYFILE) > $(DOXYFILE).tmp
	@echo PROJECT_NUMBER = $(VDRVERSION) >> $(DOXYFILE).tmp
	$(DOXYGEN) $(DOXYFILE).tmp
	@rm $(DOXYFILE).tmp

# Housekeeping:

clean:
	@$(MAKE) --no-print-directory -C $(LSIDIR) clean
	@-rm -f $(COBJS) $(CXXOBJS) $(DEPFILE) vdr.bin libvdr.so vdr.pc core* *~
	@-rm -rf $(LOCALEDIR) $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -rf include
	@-rm -rf srcdoc
CLEAN: clean
distclean: clean-plugins clean
