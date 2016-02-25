plugindir = "$(libdir)/mailtc"

plugin_LTLIBRARIES = net.la

AM_CPPFLAGS = \
		$(GLIB_CFLAGS) \
		-I$(top_srcdir)/src \
		-DG_DISABLE_DEPRECATED=1 \
		-DLIBDIR=\"$(plugindir)\"

AM_CFLAGS = \
		-Wall \
		-Wextra \
		-Wno-variadic-macros \
		-ansi \
		-pedantic

if ENABLE_DEBUG
AM_CPPFLAGS += \
		-DDEBUG

AM_CFLAGS += \
		-ggdb
endif

LDADD = \
		$(GLIB_LIBS)

net_la_SOURCES = \
		mtc-socket.c \
		mtc-uid.c \
		mtc-net.c \
		mtc-pop.c

net_la_LDFLAGS = \
		-no-undefined \
		-module \
		-avoid-version \
		-rpath $(plugindir)

EXTRA_DIST = \
		mtc-socket.h \
		mtc-uid.h \
		mtc-net.h

