dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/main.c)

PACKAGE=mailtc
VERSION=1.0.4

AM_INIT_AUTOMAKE([$PACKAGE], [$VERSION])

dnl Checks for programs.
AC_LANG_C
AC_PROG_CC
AC_AIX
AC_ISC_POSIX
AC_MINIX
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl CFLAGS="-Wall -W -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED"
CFLAGS=""

dnl Adds an option to enable debugging
AC_ARG_ENABLE(debug,AS_HELP_STRING([--enable-debug],[Compile mailtc with debugging enabled]),[debug=yes],[debug=no])
dnl if test x$debug == xyes ; then
dnl	AC_DEFINE(MTC_DEBUG,1)
dnl fi
AC_MSG_CHECKING([if debugging should be enabled])
AC_MSG_RESULT($debug)
AM_CONDITIONAL(DEBUG, test x$debug = xyes)

dnl Checks for libraries.
AM_PATH_GTK_2_0(2.4.0,,AC_MSG_ERROR([mailtc needs GTK+ 2.4 or higher]))

dnl Checks for header files.
AC_CHECK_HEADERS(limits.h unistd.h netdb.h sys/types.h netinet/in.h sys/socket.h sys/stat.h sys/select.h glib.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(fork execlp access rename remove mkdir socket gethostbyname connect send recv memset fopen time strstr select kill)

dnl the available languages
MAILTC_LANG=""

dnl Check whether to enable gettext
AC_ARG_ENABLE(nls,AS_HELP_STRING([--disable-nls],[don't use NLS]),[nls=no],[nls=yes])
if test $nls == yes ; then
	ALL_LINGUAS="$MAILTC_LANG"
	AM_GLIB_GNU_GETTEXT
else
	ALL_LINGUAS=""
	USE_NLS="no"
fi
AC_MSG_CHECKING([if NLS should be used])
AC_MSG_RESULT($nls)
AM_CONDITIONAL(MTC_NLS, test x$nls = xyes)
GETTEXT_PACKAGE=[$PACKAGE]
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [gettext domain])
dnl we should probably add a conditional to ignore PO dir

dnl Check whether to enable OpenSSL
AC_ARG_ENABLE(ssl,AS_HELP_STRING([--disable-ssl],[don't use OpenSSL]),[ssl=no],[ssl=yes])
if test $ssl == yes ; then
	AC_CHECK_HEADER([openssl/evp.h],[ssl=yes],[ssl=no])
	if test $ssl != yes ; then
		AC_MSG_WARN([Cannot find OpenSSL headers, disabling])
	else
		AC_DEFINE(MTC_USE_SSL,1)
	fi
fi
AC_MSG_CHECKING([if OpenSSL should be used])
AC_MSG_RESULT($ssl)
AM_CONDITIONAL(MTC_SSL, test x$ssl = xyes)

dnl Check whether to enable SASL
AC_ARG_ENABLE(sasl,AS_HELP_STRING([--disable-sasl],[don't use libgsasl]),[sasl=no],[sasl=yes])
if test $sasl == yes ; then
	AC_CHECK_HEADER([gsasl.h],[sasl=yes],[sasl=no])
	if test $sasl != yes ; then
		AC_MSG_WARN([Cannot find libgsasl headers, disabling])
	else
		AC_DEFINE(MTC_USE_SASL,1)
	fi
fi
AC_MSG_CHECKING([if libgsasl should be used])
AC_MSG_RESULT($sasl)
AM_CONDITIONAL(MTC_SASL, test x$sasl = xyes)

AC_SUBST(VERSION)

AC_OUTPUT([Makefile src/Makefile po/Makefile.in])

echo "
Configuration for $PACKAGE $VERSION:
------------------------------------

 Compiler/Linker .................. : $CC
 GLIB version ..................... : `pkg-config --modversion gtk+-2.0`
 GTK+ version ..................... : `pkg-config --modversion glib-2.0`
 NLS/gettext ...................... : $USE_NLS
 SASL ............................. : $sasl
 SSL .............................. : $ssl
 Debug mode ....................... : $debug

Now type 'make' to build $PACKAGE $VERSION,
and then type 'make install' for installation.
"
