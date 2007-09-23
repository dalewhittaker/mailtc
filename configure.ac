dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/main.c)

PACKAGE=mailtc
VERSION=1.3.9

dnl set $target
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([$PACKAGE], [$VERSION])

dnl Checks for programs.
AC_LANG_C
AC_PROG_CC
AC_AIX
AC_ISC_POSIX
AC_MINIX
AC_HEADER_STDC
AM_DISABLE_STATIC
AM_PROG_LIBTOOL
dnl prevent libtool outputting lots of messages
LIBTOOL="$LIBTOOL --silent"
dnl we only want shared libs
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl CFLAGS="-Wall -W -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED"
CFLAGS=""

dnl add OS specific stuff
mingw=no
case "$target" in
*-*-mingw*)
    mingw=yes
    CFLAGS="$CFLAGS --mms-bitfields"
    ;;
esac

dnl Adds an option to enable debugging
AC_ARG_ENABLE(debug,AS_HELP_STRING([--enable-debug],[Compile mailtc with debugging enabled]),[debug=yes],[debug=no])
dnl if test x$debug == xyes ; then
dnl	AC_DEFINE(MTC_DEBUG,1)
dnl fi
AC_MSG_CHECKING([if debugging should be enabled])
AC_MSG_RESULT($debug)
AM_CONDITIONAL(DEBUG, test x$debug = xyes)

dnl Checks for libraries.
dnl AM_PATH_GTK_2_0(2.8.0,,AC_MSG_ERROR([mailtc needs GTK+ 2.8 or higher]))
dnl next line needs fixing
dnl AM_PATH_XML2(2.6.27,,AC_MSG_ERROR([mailtc needs libxml2 2.6.27 or higher]))
dnl PKG_CHECK_MODULES([LIBXML2], [libxml-2.0 >= 2.6.0])
dnl AC_SUBST(LIBXML_CFLAGS)
dnl AC_SUBST(LIBXML_LIBS)
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.10.0  libxml-2.0 >= 2.4.0)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

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
AC_ARG_ENABLE(nls,AS_HELP_STRING([--disable-nls],[do not use NLS]),[nls=no],[nls=yes])
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
AC_ARG_ENABLE(ssl,AS_HELP_STRING([--disable-ssl],[do not build OpenSSL plugins]),[ssl=no],[ssl=yes])
if test $ssl == yes ; then
	AC_CHECK_HEADER([openssl/evp.h],[ssl=yes],[ssl=no])
	if test $ssl != yes ; then
		AC_MSG_WARN([Cannot find OpenSSL headers, disabling])
	else
		AC_DEFINE(MTC_USE_SSL,1)
	fi
fi
AC_MSG_CHECKING([if OpenSSL plugins should be built])
AC_MSG_RESULT($ssl)
AM_CONDITIONAL(MTC_SSL, test x$ssl = xyes)

dnl Check whether to enable POP3 plugins 
AC_ARG_ENABLE(pop,AS_HELP_STRING([--disable-pop],[do not build pop3 plugins]),[pop=no],[pop=yes])
if test $pop == yes ; then
	AC_DEFINE(MTC_USE_POP,1)
fi
AC_MSG_CHECKING([if pop3 plugins should be built])
AC_MSG_RESULT($pop)
AM_CONDITIONAL(MTC_POP, test x$pop = xyes)

dnl Check whether to enable IMAP4 plugins 
AC_ARG_ENABLE(imap,AS_HELP_STRING([--disable-imap],[do not build imap4 plugins]),[imap=no],[imap=yes])
if test $imap == yes ; then
	AC_DEFINE(MTC_USE_IMAP,1)
fi
AC_MSG_CHECKING([if imap4 plugins should be built])
AC_MSG_RESULT($imap)
AM_CONDITIONAL(MTC_IMAP, test x$imap = xyes)

dnl Check whether to enable the message list
AC_ARG_ENABLE(minimal,AS_HELP_STRING([--enable-minimal],[compile with minimal features enabled. Only use if you disable imap.]),[minimal=yes],[minimal=no])
if test $minimal == no ; then
       AC_DEFINE(MTC_NOTMINIMAL,1)
else	 
    if test $imap == yes ; then
        AC_MSG_ERROR([Minimal cannot be compiled with imap enabled, as it requires extra features.  Either disable minimal build or disable imap using --disable-imap])
    fi
fi
AC_MSG_CHECKING([whether to do a minimal build])
AC_MSG_RESULT($minimal)
AM_CONDITIONAL(MTC_ALLFEATURES, test x$minimal = xno)

dnl set the define for WIN32
if test $mingw == no ; then
       AC_DEFINE(MTC_EGGTRAYICON,1)
fi
AC_MSG_CHECKING([whether to compile for Win32])
AC_MSG_RESULT($mingw)
AM_CONDITIONAL(MTC_EGG, test x$mingw = xno)

AC_SUBST(VERSION)

AC_OUTPUT([Makefile src/Makefile plugin/Makefile po/Makefile.in])

echo "
Configuration for $PACKAGE $VERSION:
------------------------------------

 Compiler/Linker .................. : $CC
 GTK+ version ..................... : `pkg-config --modversion gtk+-2.0`
 Glib version ..................... : `pkg-config --modversion glib-2.0`
 libxml2 version .................. : `pkg-config --modversion libxml-2.0`
 NLS/gettext ...................... : $USE_NLS
 SSL .............................. : $ssl
 POP .............................. : $pop
 IMAP ............................. : $imap
 Debug mode ....................... : $debug
 Minimal build .....................: $minimal
 
Now type 'make' to build $PACKAGE $VERSION,
and then type 'make install' for installation.
"
