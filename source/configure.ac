AC_PREREQ(2.57)

m4_define([mailtc_package],[mailtc])
m4_define([mailtc_version],[1.4.3])

m4_define([glib_required_version],[2.20.0])
m4_define([gmodule_required_version],[2.20.0])
m4_define([gtk_required_version],[2.16.0])
m4_define([gnutls_required_version],[2.6.6])
m4_define([unique_required_version],[1.1.2])

AC_INIT([mailtc_package],[mailtc_version],[dale@users.sf.net])

AC_CONFIG_MACRO_DIR([autotools])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_SRCDIR([src/mtc-config.h])
AC_SUBST(ACLOCAL_AMFLAGS,"-I autotools")

# for autoheader
AC_CONFIG_HEADERS([config.h])

GLIB_REQUIRED_VERSION=glib_required_version
GMODULE_REQUIRED_VERSION=gmodule_required_version
GTK_REQUIRED_VERSION=gtk_required_version
UNIQUE_REQUIRED_VERSION=unique_required_version
GNUTLS_REQUIRED_VERSION=gnutls_required_version

AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GMODULE_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)
AC_SUBST(GNUTLS_REQUIRED_VERSION)
AC_SUBST(UNIQUE_REQUIRED_VERSION)

# put .o files in the subdirectory and package as bzip2
AM_INIT_AUTOMAKE([subdir-objects dist-bzip2])

if test -z "$CFLAGS"; then
    CFLAGS=""
fi

# determine a C compiler to use
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_C_CONST
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

PKG_PROG_PKG_CONFIG(0,23)
PKG_CHECK_MODULES(GLIB,glib-2.0 >= $GLIB_REQUIRED_VERSION)
PKG_CHECK_MODULES(GMODULE,gmodule-2.0 >= $GMODULE_REQUIRED_VERSION)
PKG_CHECK_MODULES(GTK,gtk+-2.0 >= $GTK_REQUIRED_VERSION)
PKG_CHECK_MODULES(UNIQUE,unique-1.0 >= $UNIQUE_REQUIRED_VERSION)

AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])
AC_SUBST([UNIQUE_CFLAGS])
AC_SUBST([UNIQUE_LIBS])

AC_ARG_ENABLE(ssl,
              AC_HELP_STRING([--disable-ssl],
                             [do not build GnuTLS plugins [default=no]]),
              [],[enable_ssl="yes"])
if test "$enable_ssl" = "yes" ; then
    PKG_CHECK_MODULES(GNUTLS,gnutls >= $GNUTLS_REQUIRED_VERSION,
                      [], [enable_ssl="no"])
    if test "$enable_ssl" = "no" ; then
        AC_MSG_WARN([Cannot find GnuTLS headers, disabling])
    else
        AC_DEFINE(HAVE_GNUTLS, 1, [Defined if you have GnuTLS support])
        AC_SUBST([GNUTLS_CFLAGS])
        AC_SUBST([GNUTLS_LIBS])
    fi
fi

AM_CONDITIONAL([HAVE_GNUTLS],[test "$enable_ssl" = "yes"])

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
Makefile
])

AC_OUTPUT

AC_MSG_RESULT([
mailtc was configured with the following options:

Build configuration:
    Enable debugging : $enable_debug
    Enable GnuTLS    : $enable_ssl

Now type 'make' to build $PACKAGE $VERSION,
and then type 'make install' for installation.
])

