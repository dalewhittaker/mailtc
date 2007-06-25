#AUTOMAKE_OPTIONS = gnu
bin_PROGRAMS = mailtc

mtc_include = $(top_srcdir)/include

if MTC_SSL
ssl_libraries = -lcrypto -lssl
ssl_sources = encrypter.c
ssl_headers = $(mtc_include)/encrypter.h
endif

mtc_cflags = $(CFLAGS) -Wall -W -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED 

#extra cflags are for the gettext stuff, and also the plugin dir
extra_cflags = -DLOCALE_DIR=\""$(datadir)/locale"\" \
				-DLIBDIR=\""$(libdir)/mailtc"\" \
				-DPACKAGE=\"$(PACKAGE)\" \
				-DVERSION=\"$(VERSION)\"

if MTC_ALLFEATURES
msglist_cflags = -DMTC_NOTMINIMAL
msglist_sources = filterdlg.c
msglist_headers = $(mtc_include)/envelope_small.h \
				$(mtc_include)/filterdlg.h
endif

if DEBUG
debug_cflags = -g -DDEBUG
endif

if MTC_EGG
egg_sources = eggtrayicon.c
egg_headers = $(mtc_include)/eggtrayicon.h
endif

AM_CFLAGS = $(mtc_cflags) \
			$(extra_cflags) \
			$(msglist_cflags) \
			$(debug_cflags)

#Here we are defining out libraries
LDADD = @GTK_LIBS@ \
		$(ssl_libraries)

#$(sasl_libraries)

INCLUDES = -I$(mtc_include) @GTK_CFLAGS@
	
#define the various builds
mailtc_SOURCES = configdlg.c \
	docklet.c \
	filefunc.c \
	common.c \
	plugfunc.c \
	$(ssl_sources) \
	$(msglist_sources) \
	$(egg_sources) \
	main.c

#TODO summary removed for now as it is broken
#summarydlg.c
#summarydlg.h

noinst_HEADERS = $(mtc_include)/common.h \
		$(mtc_include)/configdlg.h \
		$(mtc_include)/docklet.h \
		$(mtc_include)/filefunc.h \
		$(mtc_include)/plugfunc.h \
		$(mtc_include)/strings.h \
		$(mtc_include)/envelope_large.h \
		$(mtc_include)/plugin.h \
		$(ssl_headers) \
		$(egg_headers) \
		$(msglist_headers)
		
