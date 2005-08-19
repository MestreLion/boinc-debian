#!/usr/bin/make -f

# Uncomment this to turn on verbose mode.
#export DH_VERBOSE=1

DEB_BUILD_GNU_TYPE := $(shell dpkg-architecture -qDEB_BUILD_GNU_TYPE)

# Most BOINC projects only provide applications for i686.
ifeq ($(DEB_BUILD_GNU_TYPE), i486-linux-gnu)
  DEB_BUILD_GNU_TYPE = i686-linux-gnu
endif

CFLAGS += -g -Wall
CXXFLAGS += -g -Wall

CFLAGS_boinc-client := $(CFLAGS)
CXXFLAGS_boinc-client := $(CXXFLAGS)

ifneq (,$(findstring noopt,$(DEB_BUILD_OPTIONS)))
  DEB_OPT_FLAG = -O0
  DEB_OPT_FLAG_boinc-client = -O0
else
  DEB_OPT_FLAG = -O2
  DEB_OPT_FLAG_boinc-client = -O3 -ffast-math
endif

CFLAGS += $(DEB_OPT_FLAG)
CXXFLAGS += $(DEB_OPT_FLAG)

CFLAGS_boinc-client += $(DEB_OPT_FLAG_boinc-client)
CXXFLAGS_boinc-client += $(DEB_OPT_FLAG_boinc-client)

CFGFLAGS = --prefix=/usr --enable-client --enable-server CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)"
CFGFLAGS_boinc-client = --build=$(DEB_BUILD_GNU_TYPE) --enable-client --disable-server CFLAGS="$(CFLAGS_boinc-client)" CXXFLAGS="$(CXXFLAGS_boinc-client)"

build: build-stamp
build-stamp: build-stamp-boinc-client
	dh_testdir
	./configure $(CFGFLAGS)
	$(MAKE)
	touch $@

build-stamp-boinc-client:
	dh_testdir
	./configure $(CFGFLAGS_boinc-client)
	$(MAKE)
	cp client/boinc_client client/boinc_client.optimized
	touch $@

clean: clean-boinc-client
	dh_testdir
	dh_testroot
	rm -f build-stamp
	-$(MAKE) clean
	-$(MAKE) distclean
	
	dh_clean boinc_client.1 boinc_cmd.1 boincmgr.1
	dh_clean api/Makefile.in apps/Makefile.in client/Makefile.in \
	  clientgui/Makefile.in db/Makefile.in lib/Makefile.in \
	  m4/Makefile.in py/Makefile.in py/Boinc/Makefile.in \
	  sched/Makefile.in sched/status sched/stop sea/Makefile.in \
	  test/Makefile.in tools/Makefile.in zip/Makefile.in \
	  zip/unzip/Makefile.in zip/zip/Makefile.in Makefile.in \
	  aclocal.m4 config.h.in configure

clean-boinc-client:
	dh_testdir
	dh_testroot
	rm -f build-stamp-boinc-client
	dh_clean client/boinc_client.optimized
