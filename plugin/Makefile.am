#plugin Makefile

#define the directory to install to
plugindir = "$(libdir)/mailtc"
includedir = $(top_srcdir)/include

#define the common source files
commonsrc = plg_common.c netfunc.c
popsrc = $(commonsrc) popfunc.c
imapsrc = $(commonsrc) imapfunc.c

if MTC_SSL
ssl_cflags = -DSSL_PLUGIN
ssl_libfiles = \
	mtc_apop.la \
	mtc_popcram.la \
	mtc_imapcram.la \
	mtc_popssl.la \
	mtc_imapssl.la
endif

if DEBUG
debug_cflags = -g -DDEBUG
endif

#define the common cflags
plg_cflags= -Wall -W \
			-DPACKAGE=\"$(PACKAGE)\" \
			-DVERSION=\"$(VERSION)\"

common_ldflags = -no-undefined -module -avoid-version -rpath $(plugindir)

INCLUDES = -I$(includedir)

#define the plugins
mtc_pop_la_CFLAGS = $(plg_cflags) $(debug_cflags)
mtc_pop_la_LDFLAGS = $(common_ldflags)
mtc_imap_la_CFLAGS = $(plg_cflags) $(debug_cflags)
mtc_imap_la_LDFLAGS = $(common_ldflags)

#use noinst, as we do not want to install the actual .la files
plugin_LTLIBRARIES = \
	mtc_pop.la \
	mtc_imap.la \
	$(ssl_libfiles)

mtc_pop_la_SOURCES = $(popsrc) plg_pop.c
mtc_imap_la_SOURCES = $(imapsrc) plg_imap.c

#if SSL is defined we want to build the extra plugins
if MTC_SSL
ssl_libraries = -lcrypto -lssl
mtc_popcram_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags)
mtc_popcram_la_LDFLAGS = $(common_ldflags)
mtc_popcram_la_LIBADD = $(ssl_libraries)
mtc_popcram_la_SOURCES = $(popsrc) plg_popcram.c

mtc_imapcram_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags)
mtc_imapcram_la_LDFLAGS = $(common_ldflags)
mtc_imapcram_la_LIBADD = $(ssl_libraries)
mtc_imapcram_la_SOURCES = $(imapsrc) plg_imapcram.c

mtc_popssl_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags)
mtc_popssl_la_LDFLAGS = $(common_ldflags)
mtc_popssl_la_LIBADD = $(ssl_libraries)
mtc_popssl_la_SOURCES = $(popsrc) plg_popssl.c

mtc_apop_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags)
mtc_apop_la_LDFLAGS = $(common_ldflags) $(ssl_libraries)
mtc_apop_la_SOURCES = $(popsrc) plg_apop.c
mtc_apop_la_LIBADD = $(ssl_libraries)

mtc_imapssl_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags)
mtc_imapssl_la_LDFLAGS = $(common_ldflags) $(ssl_libraries)
mtc_imapssl_la_LIBADD = $(ssl_libraries)
mtc_imapssl_la_SOURCES = $(imapsrc) plg_imapssl.c
endif

noinst_HEADERS = \
	$(includedir)/plugin.h \
	$(includedir)/plg_strings.h \
	$(includedir)/plg_common.h

