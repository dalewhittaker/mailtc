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

mailtc_SOURCES = \
		mtc-application.c \
		mtc-config.c \
		mtc-envelope.c \
		mtc-file.c \
		mtc-mail.c \
		mtc-module.c \
		mtc-statusicon.c \
		mtc-util.c \
		main.c

EXTRA_DIST = \
		mtc.h \
		mtc-application.h \
		mtc-config.h \
		mtc-envelope.h \
		mtc-file.h \
		mtc-mail.h \
		mtc-module.h \
		mtc-statusicon.h \
		mtc-util.h
