dnl dummy plugin configure file
AC_INIT(plg_dummy.c)

PACKAGE=mtc_dummyplug
dnl the VERSION should match the mailtc version it is to be used with
VERSION=1.3.7

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
AC_PROG_INSTALL
AC_PROG_MAKE_SET

CFLAGS=""

AM_PATH_GTK_2_0(2.4.0,,AC_MSG_ERROR([mailtc needs GTK+ 2.4 or higher]))
dnl Checks for header files.
AC_CHECK_HEADERS(limits.h unistd.h netdb.h sys/types.h netinet/in.h sys/socket.h sys/stat.h sys/select.h glib.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(fork execlp access rename remove mkdir socket gethostbyname connect send recv memset fopen time strstr select kill)

AC_SUBST(VERSION)

AC_OUTPUT([Makefile])
