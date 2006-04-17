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

#Here we are defining out libraries
LDADD = @GTK_LIBS@ $(ssl_libraries) $(sasl_libraries)

INCLUDES = @GTK_CFLAGS@


#define the various builds
mailtc_SOURCES = eggtrayicon.c configdlg.c docklet.c \
	filefunc.c common.c netfunc.c popfunc.c imapfunc.c filterdlg.c \
	$(ssl_sources) $(sasl_sources) main.c

noinst_HEADERS = eggtrayicon.h header.h strings.h \
	envelope_white.h envelope_small.h

