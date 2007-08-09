#dummy plugin Makefile

#define the directory to install to
plugindir = "$(libdir)/mailtc"

#define the common cflags
INCLUDES = -I$(top_srcdir) @GTK_CFLAGS@

plugin_LTLIBRARIES = mtc_dummyplug.la

mtc_dummyplug_la_CFLAGS= $(CFLAGS) \
			-g \
			-Wall -W \
			-DGTK_DISABLE_DEPRECATED \
			-DGDK_DISABLE_DEPRECATED \
			-DGDK_PIXBUF_DISABLE_DEPRECATED \
			-DG_DISABLE_DEPRECATED \
			-DMTC_EGGTRAYICON=1 \
			-DMTC_NOTMINIMAL=1 \
			-DPACKAGE=\"$(PACKAGE)\" \
			-DVERSION=\"$(VERSION)\"


AM_LDFLAGS = -no-undefined -module -avoid-version -rpath $(plugindir)
mtc_dummyplug_la_LDFLAGS = -no-undefined -module -avoid-version -rpath $(plugindir)
mtc_dummyplug_la_LIBADD = @GTK_LIBS@
mtc_dummyplug_la_SOURCES = plg_dummy.c

noinst_HEADERS = plugin.h \
	eggtrayicon.h
