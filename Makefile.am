AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cpk

cpk_SOURCES = src/cpk.cpp src/utils.cpp \
              src/commands/cmd_update.cpp \
              src/commands/cmd_info.cpp \
              src/commands/cmd_deps.cpp \
              src/commands/cmd_deptree.cpp \
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
# All headers used by the tree must be listed so `make dist` includes them.
noinst_HEADERS = src/cpk.h src/utils.h \
              src/commands/cmd_archive.h \
              src/commands/cmd_build.h \
              src/commands/cmd_clean.h \
              src/commands/cmd_deps.h \
              src/commands/cmd_deptree.h \
              src/commands/cmd_diff.h \
              src/commands/cmd_index.h \
              src/commands/cmd_info.h \
              src/commands/cmd_install.h \
              src/commands/cmd_list.h \
              src/commands/cmd_search.h \
              src/commands/cmd_uninstall.h \
              src/commands/cmd_update.h \
              src/commands/cmd_upgrade.h \
              src/commands/cmd_verify.h

AM_CPPFLAGS = -O2 -pipe -Isrc
AM_CXXFLAGS = 

cpk_CPPFLAGS = $(AM_CPPFLAGS) $(LIBARCHIVE_CFLAGS) $(LIBCURL_CFLAGS)
cpk_LDADD = $(LIBARCHIVE_LIBS) $(LIBCURL_LIBS)

sysconfdir = /etc
sysconf_DATA = cpk.conf

man_MANS     = man/cpk.1
EXTRA_DIST   = $(man_MANS) cpk.conf.in

# Ship a default cpk.conf in the tarball (same bytes as cpk.conf.in); configure
# still generates cpk.conf from cpk.conf.in in the build tree.
dist-hook:
	cp '$(srcdir)/cpk.conf.in' '$(distdir)/cpk.conf'
