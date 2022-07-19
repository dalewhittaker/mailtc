GTESTER = gtester

SRC_DIR = $(top_srcdir)/src

EXTRA_DIST =

TEST_PROGS =

AM_CPPFLAGS = \
		$(GLIB_CFLAGS) \
		$(GTK_CFLAGS) \
		-I$(SRC_DIR) \
		-DGDK_MULTIHEAD_SAFE=1 \
		-DGTK_MULTIHEAD_SAFE=1 \
		-DPLUGINDIR=\""$(top_srcdir)/plugin/.libs"\" \
		-DLIBDIR=\""$(libdir)/mailtc"\" \
		-DSRCDIR=\""$(srcdir)"\" \
		-DDEBUG

AM_CFLAGS = \
		-Wall \
		-Wextra \
		-Wno-variadic-macros \
		-ansi \
		-ggdb

LDADD = \
		$(GLIB_LIBS) \
		$(GTK_LIBS)

TEST_PROGS += test-pop

test_pop_SOURCES = \
		$(SRC_DIR)/mtc-account.c \
		$(SRC_DIR)/mtc-colour.c \
		$(SRC_DIR)/mtc-extension.c \
		$(SRC_DIR)/mtc-module.c \
		$(SRC_DIR)/mtc-util.c \
		test-pop.c

noinst_PROGRAMS = $(TEST_PROGS)

dotest: $(TEST_PROGS)
	$(GTESTER) --verbose $(TEST_PROGS)

check-local: dotest

