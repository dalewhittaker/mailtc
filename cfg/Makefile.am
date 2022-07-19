bin_PROGRAMS = mailtc-config

RES_DIR = $(top_srcdir)/resource

SRC_DIR = $(top_srcdir)/src

AM_CPPFLAGS = \
		$(GLIB_CFLAGS) \
		$(GTK_CFLAGS) \
		-I$(top_srcdir)/src \
		-DGDK_MULTIHEAD_SAFE=1 \
		-DGTK_MULTIHEAD_SAFE=1 \
		-DLIBDIR=\""$(libdir)/mailtc"\"

AM_CFLAGS = \
		-Wall \
		-Wextra \
		-ansi

if ENABLE_DEBUG
AM_CPPFLAGS += \
		-DDEBUG

AM_CFLAGS += \
		-g
endif

LDADD = \
		$(GLIB_LIBS) \
		$(GTK_LIBS)

RES_FILES = \
		$(RES_DIR)/mtc-configdialog.ui \
		$(RES_DIR)/mtc-envelope.png

mtc-resources.c: mtc-gresource.xml $(RES_FILES)
		glib-compile-resources --sourcedir=$(RES_DIR) --target=$@ --generate-source --c-name mailtc_config mtc-gresource.xml

mtc-resources.h: mtc-gresource.xml $(RES_FILES)
		glib-compile-resources --sourcedir=$(RES_DIR) --target=$@ --generate-header --c-name mailtc_config mtc-gresource.xml

mailtc_config_SOURCES = \
		$(SRC_DIR)/mtc-account.c \
		$(SRC_DIR)/mtc-application.c \
		$(SRC_DIR)/mtc-colour.c \
		$(SRC_DIR)/mtc-extension.c \
		$(SRC_DIR)/mtc-module.c \
		$(SRC_DIR)/mtc-modulemanager.c \
		$(SRC_DIR)/mtc-pixbuf.c \
		$(SRC_DIR)/mtc-settings.c \
		$(SRC_DIR)/mtc-util.c \
		mtc-resources.c \
		mtc-envelope.c \
		mtc-configdialog.c \
		main.c

EXTRA_DIST = \
		$(RES_FILES) \
		mtc-resources.h \
		mtc-envelope.h \
		mtc-configdialog.h
