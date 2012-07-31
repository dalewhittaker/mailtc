plugindir = "$(libdir)/mailtc"

plugin_LTLIBRARIES = pop.la

AM_CPPFLAGS = \
		-DATK_DISABLE_SINGLE_INCLUDES \
		-DATK_DISABLE_DEPRECATED \
		-DG_DISABLE_SINGLE_INCLUDES \
		-DG_DISABLE_DEPRECATED \
		-DGTK_DISABLE_SINGLE_INCLUDES \
		-DGTK_DISABLE_DEPRECATED \
		-DGTK_MULTIHEAD_SAFE \
		-DGDK_PIXBUF_DISABLE_DEPRECATED \
		-DGDK_DISABLE_DEPRECATED \
		-DGDK_MULTIHEAD_SAFE \
		-DPANGO_DISABLE_DEPRECATED \
		-DGSEAL_ENABLE \
		-DLIBDIR=\"$(plugindir)\"


AM_CFLAGS = \
		-Wall \
		-Wextra \
		-ansi \
		-pedantic

if ENABLE_DEBUG
AM_CPPFLAGS += \
		-DDEBUG

AM_CFLAGS += \
		-ggdb
endif

INCLUDES = \
		$(GTK_CFLAGS) \
		-I$(top_srcdir)/src

LDADD = \
		$(GTK_LIBS)

pop_la_SOURCES = \
		mtc-socket.c \
		mtc-uid.c \
		mtc-net.c \
		mtc-pop.c

pop_la_LDFLAGS = \
		-no-undefined \
		-module \
		-avoid-version \
		-rpath $(plugindir)

EXTRA_DIST = \
		mtc-socket.h \
		mtc-uid.h \
		mtc-net.h

