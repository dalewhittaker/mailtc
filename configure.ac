AC_PREREQ(2.57)

m4_define([mailtc_package],[mailtc])
m4_define([mailtc_version],[1.4.3])

m4_define([glib_required_version],[GLIB_VERSION_2_72])
m4_define([gdk_pixbuf_required_version],[2.0.0])
m4_define([gtk_required_version],[3.16.0])

AC_INIT([mailtc_package],[mailtc_version],[dale@users.sf.net])

AC_CONFIG_MACRO_DIR([autotools])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_SRCDIR([src/mtc-util.h])
AC_SUBST(ACLOCAL_AMFLAGS,"-I autotools")

# for autoheader
AC_CONFIG_HEADERS([config.h])

# put .o files in the subdirectory and package as bzip2
AM_INIT_AUTOMAKE([subdir-objects dist-bzip2 no-dist-gzip])

# enable silent automake if available
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# determine a C compiler to use
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_C_CONST
AC_HEADER_STDC
AC_DISABLE_STATIC
LT_INIT
LIBTOOL="$LIBTOOL --silent"
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

PKG_PROG_PKG_CONFIG(0,23)
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= gtk_required_version])
PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= gdk_pixbuf_required_version])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= glib_required_version, gobject-2.0, gmodule-2.0, gio-2.0])

AC_SUBST([GLIB_CFLAGS])
AC_SUBST([GLIB_LIBS])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

AC_DEFINE([GLIB_VERSION_MIN_REQUIRED], [glib_required_version], [Ignore post 2.44 deprecations])

AC_MSG_CHECKING([whether to do a debug build])
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [turn on debugging [default=no]]),
              [],[enable_debug="no"])
AC_MSG_RESULT([$enable_debug])

AM_CONDITIONAL([ENABLE_DEBUG],[test "$enable_debug" = "yes"])

AC_CONFIG_FILES([
src/Makefile
plugin/Makefile
cfg/Makefile
tests/Makefile
Makefile
])

AC_OUTPUT

AC_MSG_RESULT([
mailtc was configured with the following options:

Build configuration:
    Enable debugging : $enable_debug

Now type 'make' to build $PACKAGE $VERSION,
and then type 'make install' for installation.
])

