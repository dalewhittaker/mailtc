#AUTOMAKE_OPTIONS = gnu
bin_PROGRAMS = mailtc

if SSL
ssl_libraries = -lcrypto -lssl
ssl_sources = encrypter.c tls.c
endif

if SASL
sasl_libraries = -lgsasl
sasl_sources = sasl.c
endif

mtc_cflags = -Wall -W -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED 
locale_cflags = -DLOCALE_DIR=\""$(datadir)/locale"\"
mtc_include = $(top_srcdir)/include

if DEBUG
AM_CFLAGS = $(mtc_cflags) $(locale_cflags) -g -DDEBUG
else
AM_CFLAGS = $(mtc_cflags) $(locale_cflags)
endif

#Here we are defining out libraries
LDADD = @GTK_LIBS@ $(ssl_libraries) $(sasl_libraries)

INCLUDES = -I$(mtc_include) @GTK_CFLAGS@ 
	
#define the various builds
mailtc_SOURCES = eggtrayicon.c configdlg.c docklet.c \
	filefunc.c common.c netfunc.c popfunc.c imapfunc.c filterdlg.c \
	$(ssl_sources) $(sasl_sources) main.c

noinst_HEADERS = $(mtc_include)/eggtrayicon.h $(mtc_include)/core.h \
		$(mtc_include)/strings.h $(mtc_include)/envelope_white.h \
		$(mtc_include)/envelope_small.h

