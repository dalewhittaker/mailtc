AC_PREREQ(2.57)

m4_define([mailtc_package],[mailtc])
m4_define([mailtc_version],[1.4.3])

m4_define([glib_required_version],[2.28.0])
m4_define([gmodule_required_version],[2.28.0])
m4_define([gthread_required_version],[2.28.0])
m4_define([gtk_required_version],[2.24.0])

AC_INIT([mailtc_package],[mailtc_version],[dale@users.sf.net])

AC_CONFIG_MACRO_DIR([autotools])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_SRCDIR([src/mtc-config.h])
AC_SUBST(ACLOCAL_AMFLAGS,"-I autotools")

# for autoheader
AC_CONFIG_HEADERS([config.h])

GLIB_REQUIRED_VERSION=glib_required_version
GMODULE_REQUIRED_VERSION=gmodule_required_version
GTHREAD_REQUIRED_VERSION=gthread_required_version
GTK_REQUIRED_VERSION=gtk_required_version

AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GMODULE_REQUIRED_VERSION)
AC_SUBST(GTHREAD_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)

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
PKG_CHECK_MODULES(GLIB,glib-2.0 >= $GLIB_REQUIRED_VERSION)
PKG_CHECK_MODULES(GMODULE,gmodule-2.0 >= $GMODULE_REQUIRED_VERSION)
PKG_CHECK_MODULES(GTHREAD,gthread-2.0 >= $GTHREAD_REQUIRED_VERSION)
PKG_CHECK_MODULES(GTK,gtk+-2.0 >= $GTK_REQUIRED_VERSION)

AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])
AC_SUBST([GTHREAD_LIBS])

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

