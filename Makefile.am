AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cpk

cpk_SOURCES = src/cpk.cpp src/utils.cpp \
              src/commands/cmd_update.cpp \
              src/commands/cmd_info.cpp \
              src/commands/cmd_deps.cpp \
              src/commands/cmd_search.cpp \
              src/commands/cmd_list.cpp \
              src/commands/cmd_diff.cpp \
              src/commands/cmd_verify.cpp \
              src/commands/cmd_build.cpp \
              src/commands/cmd_install.cpp \
              src/commands/cmd_uninstall.cpp \
              src/commands/cmd_upgrade.cpp \
              src/commands/cmd_clean.cpp \
              src/commands/cmd_index.cpp \
              src/commands/cmd_archive.cpp
noinst_HEADERS = src/cpk.h src/utils.h

AM_CPPFLAGS = -O2 -pipe -Isrc
AM_CXXFLAGS = 

cpk_CPPFLAGS = $(LIBARCHIVE_CFLAGS) $(LIBCURL_CFLAGS)
cpk_LDADD = $(LIBARCHIVE_LIBS) $(LIBCURL_LIBS)

sysconfdir = /etc
sysconf_DATA = cpk.conf

man_MANS     = man/cpk.1
EXTRA_DIST   = $(man_MANS)
