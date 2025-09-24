AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cpk

cpk_SOURCES = src/cpk.cpp src/utils.cpp
noinst_HEADERS = src/cpk.h src/utils.h

AM_CPPFLAGS = -O2 -pipe -Isrc
AM_CXXFLAGS = -std=c++20

cpk_CPPFLAGS = $(LIBARCHIVE_CFLAGS) $(LIBCURL_CFLAGS)
cpk_LDADD = $(LIBARCHIVE_LIBS) $(LIBCURL_LIBS)

sysconfdir = /etc
sysconf_DATA = cpk.conf

man_MANS     = man/cpk.1
EXTRA_DIST   = $(man_MANS)
