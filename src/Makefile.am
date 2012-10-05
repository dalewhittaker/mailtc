bin_PROGRAMS = mailtc

AM_CPPFLAGS = \
		-DG_DISABLE_SINGLE_INCLUDES=1 \
		-DG_DISABLE_DEPRECATED=1 \
		-DGTK_DISABLE_SINGLE_INCLUDES=1 \
		-DGTK_DISABLE_DEPRECATED=1 \
		-DGDK_PIXBUF_DISABLE_DEPRECATED=1 \
		-DGDK_DISABLE_DEPRECATED=1 \
		-DGDK_MULTIHEAD_SAFE=1 \
		-DGTK_MULTIHEAD_SAFE=1 \
		-DGSEAL_ENABLE=1 \
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
		$(GTK_LIBS)

mailtc_SOURCES = \
		mtc-account.c \
		mtc-application.c \
		mtc-checker.c \
		mtc-configdialog.c \
		mtc-envelope.c \
		mtc-extension.c \
		mtc-module.c \
		mtc-modulemanager.c \
		mtc-settings.c \
		mtc-statusicon.c \
		mtc-util.c \
		main.c

include_HEADERS = \
		mailtc.h

EXTRA_DIST = \
		mtc-account.h \
		mtc-application.h \
		mtc-checker.h \
		mtc-configdialog.h \
		mtc-envelope.h \
		mtc-extension.h \
		mtc-module.h \
		mtc-modulemanager.h \
		mtc-settings.h \
		mtc-statusicon.h \
		mtc-util.h
