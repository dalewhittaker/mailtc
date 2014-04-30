GTESTER = gtester

EXTRA_DIST =
TEST_PROGS =

AM_CPPFLAGS = \
		$(GTK_CFLAGS) \
		-I$(top_srcdir)/src \
		-DG_DISABLE_DEPRECATED=1 \
		-DGTK_DISABLE_DEPRECATED=1 \
		-DGDK_PIXBUF_DISABLE_DEPRECATED=1 \
		-DGDK_DISABLE_DEPRECATED=1 \
		-DGDK_MULTIHEAD_SAFE=1 \
		-DGTK_MULTIHEAD_SAFE=1 \
		-DPLUGINDIR=\""$(top_srcdir)/plugin/.libs"\" \
		-DLIBDIR=\""$(libdir)/mailtc"\" \
		-DSRCDIR=\""$(srcdir)"\" \
		-DDEBUG

AM_CFLAGS = \
		-Wall \
		-Wextra \
		-Wno-overlength-strings \
		-Wno-format \
		-ansi \
		-pedantic \
		-ggdb

LDADD = \
		$(GTK_LIBS)

TEST_PROGS += test-pop

test_pop_SOURCES = \
		$(top_srcdir)/src/mtc-account.c \
		$(top_srcdir)/src/mtc-application.c \
		$(top_srcdir)/src/mtc-configdialog.c \
		$(top_srcdir)/src/mtc-envelope.c \
		$(top_srcdir)/src/mtc-extension.c \
		$(top_srcdir)/src/mtc-module.c \
		$(top_srcdir)/src/mtc-modulemanager.c \
		$(top_srcdir)/src/mtc-settings.c \
		$(top_srcdir)/src/mtc-statusicon.c \
		$(top_srcdir)/src/mtc-util.c \
		test-pop.c

noinst_PROGRAMS = $(TEST_PROGS)

dotest: $(TEST_PROGS)
	$(GTESTER) --verbose $(TEST_PROGS)

check-local: dotest

