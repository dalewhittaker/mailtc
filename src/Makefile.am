bin_PROGRAMS = mailtc

AM_CPPFLAGS = \
		-DATK_DISABLE_SINGLE_INCLUDES \
		-DATK_DISABLE_DEPRECATED \
		-DG_DISABLE_SINGLE_INCLUDES \
		-DG_DISABLE_DEPRECATED \
		-DGTK_DISABLE_SINGLE_INCLUDES \
		-DGTK_DISABLE_DEPRECATED \
		-DGDK_PIXBUF_DISABLE_DEPRECATED \
		-DGDK_DISABLE_DEPRECATED \
		-DGDK_MULTIHEAD_SAFE \
		-DGTK_MULTIHEAD_SAFE \
		-DPANGO_DISABLE_DEPRECATED \
		-DGSEAL_ENABLE \
		-DLIBDIR=\""$(libdir)/mailtc"\"

AM_CFLAGS = \
		-Wall \
		-Wextra \
		-Wno-overlength-strings \
		-Wno-format \
		-ansi \
		-pedantic

if ENABLE_DEBUG
AM_CPPFLAGS += \
		-DDEBUG

AM_CFLAGS += \
		-ggdb
endif

INCLUDES = \
		$(GTK_CFLAGS)

LDADD = \
		$(GTK_LIBS) \
		$(GTHREAD_LIBS)

mtc-marshal.h: mtc-marshal.list
		glib-genmarshal --prefix=mailtc_marshal mtc-marshal.list --header > mtc-marshal.h

mtc-marshal.c: mtc-marshal.list
		glib-genmarshal --prefix=mailtc_marshal mtc-marshal.list --body > mtc-marshal.c

BUILT_SOURCES = \
		mtc-marshal.h \
		mtc-marshal.c

mailtc_SOURCES = \
		mtc-account.c \
		mtc-application.c \
		mtc-checker.c \
		mtc-configdialog.c \
		mtc-envelope.c \
		mtc-extension.c \
		mtc-marshal.c \
		mtc-module.c \
		mtc-modulemanager.c \
		mtc-settings.c \
		mtc-statusicon.c \
		mtc-util.c \
		main.c

EXTRA_DIST = \
		mailtc.h \
		mtc-marshal.list \
		mtc-account.h \
		mtc-application.h \
		mtc-checker.h \
		mtc-configdialog.h \
		mtc-envelope.h \
		mtc-extension.h \
		mtc-marshal.h \
		mtc-module.h \
		mtc-modulemanager.h \
		mtc-settings.h \
		mtc-statusicon.h \
		mtc-util.h

CLEANFILES = \
		mtc-marshal.c \
		mtc-marshal.h
