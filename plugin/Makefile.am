#plugin Makefile

#define the directory to install to
plugindir = "$(libdir)/mailtc"
includedir = $(top_srcdir)/include

#define the message list stuff
if MTC_ALLFEATURES
msglist_cflags = -DMTC_NOTMINIMAL
msglist_src = msg.c
msglist_headers = $(includedir)/msg.h
endif

#define the common source files
common_src = plg_common.c netfunc.c $(msglist_src)

#define the POP3 stuff
if MTC_POP
pop_src = $(common_src) popfunc.c
pop_libfiles = mtc_pop.la
pop_headers = $(includedir)/popfunc.h
if MTC_SSL
pop_ssl_libfiles = \
	mtc_apop.la \
	mtc_popcram.la \
	mtc_popssl.la
endif
endif

#define the IMAP4 stuff
if MTC_IMAP
imap_src = $(common_src) imapfunc.c
imap_libfiles =	mtc_imap.la
imap_headers = $(includedir)/imapfunc.h
if MTC_SSL
imap_ssl_libfiles = \
	mtc_imapcram.la \
	mtc_imapssl.la
endif
endif

#define the SSL stuff
if MTC_SSL
ssl_cflags = -DSSL_PLUGIN
ssl_libraries = -lcrypto -lssl
endif

#define the debug stuff
if DEBUG
debug_cflags = -g -DDEBUG
endif

#define the common cflags
plg_cflags = $(CFLAGS) \
			-Wall -W \
			-DGTK_DISABLE_DEPRECATED \
			-DGDK_DISABLE_DEPRECATED \
			-DGDK_PIXBUF_DISABLE_DEPRECATED \
			-DG_DISABLE_DEPRECATED \
			-DPACKAGE=\"$(PACKAGE)\" \
			-DVERSION=\"$(VERSION)\"

common_ldflags = -no-undefined -module -avoid-version -rpath $(plugindir)

INCLUDES = -I$(includedir) @GTK_CFLAGS@

#use noinst, as we do not want to install the actual .la files
plugin_LTLIBRARIES = \
	$(pop_libfiles) \
	$(imap_libfiles) \
	$(pop_ssl_libfiles) \
	$(imap_ssl_libfiles)

#we now use glib/GTK for plugins
common_libraries = @GTK_LIBS@

#define the plugins
if MTC_POP
mtc_pop_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(msglist_cflags)
mtc_pop_la_LDFLAGS = $(common_ldflags)
mtc_pop_la_LIBADD = $(common_libraries)
mtc_pop_la_SOURCES = $(pop_src) plg_pop.c
if MTC_SSL
mtc_popcram_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags) $(msglist_cflags)
mtc_popcram_la_LDFLAGS = $(common_ldflags)
mtc_popcram_la_LIBADD = $(common_libraries) $(ssl_libraries)
mtc_popcram_la_SOURCES = $(pop_src) plg_popcram.c

mtc_popssl_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags) $(msglist_cflags)
mtc_popssl_la_LDFLAGS = $(common_ldflags)
mtc_popssl_la_LIBADD = $(common_libraries) $(ssl_libraries)
mtc_popssl_la_SOURCES = $(pop_src) plg_popssl.c

mtc_apop_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags) $(msglist_cflags)
mtc_apop_la_LDFLAGS = $(common_ldflags)
mtc_apop_la_SOURCES = $(pop_src) plg_apop.c
mtc_apop_la_LIBADD = $(common_libraries) $(ssl_libraries)
endif
endif

if MTC_IMAP
mtc_imap_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(msglist_cflags)
mtc_imap_la_LDFLAGS = $(common_ldflags)
mtc_imap_la_LIBADD = $(common_libraries)
mtc_imap_la_SOURCES = $(imap_src) plg_imap.c
if MTC_SSL
mtc_imapcram_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags) $(msglist_cflags)
mtc_imapcram_la_LDFLAGS = $(common_ldflags)
mtc_imapcram_la_LIBADD = $(common_libraries) $(ssl_libraries)
mtc_imapcram_la_SOURCES = $(imap_src) plg_imapcram.c

mtc_imapssl_la_CFLAGS = $(plg_cflags) $(debug_cflags) $(ssl_cflags) $(msglist_cflags)
mtc_imapssl_la_LDFLAGS = $(common_ldflags) $(ssl_libraries)
mtc_imapssl_la_LIBADD = $(common_libraries) $(ssl_libraries)
mtc_imapssl_la_SOURCES = $(imap_src) plg_imapssl.c
endif

endif

noinst_HEADERS = \
	$(includedir)/plugin.h \
	$(includedir)/plg_strings.h \
	$(includedir)/plg_common.h \
	$(includedir)/netfunc.h \
	$(pop_headers) \
	$(imap_headers) \
	$(msglist_headers)
