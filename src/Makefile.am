APP_NAME = mailtc

RES_DIR = $(top_srcdir)/resource

bin_PROGRAMS = $(APP_NAME)

AM_CPPFLAGS = \
		$(GLIB_CFLAGS) \
		$(GTK_CFLAGS) \
		-DG_DISABLE_DEPRECATED=1 \
		-DGTK_DISABLE_DEPRECATED=1 \
		-DGDK_PIXBUF_DISABLE_DEPRECATED=1 \
		-DGDK_DISABLE_DEPRECATED=1 \
		-DGDK_MULTIHEAD_SAFE=1 \
		-DGTK_MULTIHEAD_SAFE=1 \
		-DLIBDIR=\""$(libdir)/$(APP_NAME)"\"

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

LDADD = \
		$(GLIB_LIBS) \
		$(GTK_LIBS)

RES_FILES = \
		$(RES_DIR)/mtc-envelope.png

mtc-resources.c: mtc-gresource.xml $(RES_FILES)
		glib-compile-resources --sourcedir=$(RES_DIR) --target=$@ --generate-source --c-name $(APP_NAME) mtc-gresource.xml

mtc-resources.h: mtc-gresource.xml $(RES_FILES)
		glib-compile-resources --sourcedir=$(RES_DIR) --target=$@ --generate-header --c-name $(APP_NAME) mtc-gresource.xml

mailtc_SOURCES = \
		mtc-account.c \
		mtc-application.c \
		mtc-checker.c \
		mtc-envelope.c \
		mtc-extension.c \
		mtc-module.c \
		mtc-modulemanager.c \
		mtc-resources.c \
		mtc-settings.c \
		mtc-statusicon.c \
		mtc-util.c \
		main.c

include_HEADERS = \
		mailtc.h

EXTRA_DIST = \
		$(RES_FILES) \
		mtc-account.h \
		mtc-application.h \
		mtc-checker.h \
		mtc-envelope.h \
		mtc-extension.h \
		mtc-module.h \
		mtc-modulemanager.h \
		mtc-resources.h \
		mtc-settings.h \
		mtc-statusicon.h \
		mtc-util.h
