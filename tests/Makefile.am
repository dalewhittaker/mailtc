GTESTER = gtester

EXTRA_DIST =
TEST_PROGS =

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
        -DPLUGINDIR=\""$(top_srcdir)/plugin/.libs"\" \
        -DDEBUG

AM_CFLAGS = \
        -Wall \
        -Wextra \
		-ansi \
		-pedantic \
        -ggdb

INCLUDES = \
        $(GTK_CFLAGS) \
		-I$(top_srcdir)/src

LDADD = \
        $(GTK_LIBS)

TEST_PROGS += test-pop
test_pop_SOURCES = test-pop.c

noinst_PROGRAMS = $(TEST_PROGS)

dotest: $(TEST_PROGS)
	$(GTESTER) --verbose $(TEST_PROGS)

check-local: dotest

