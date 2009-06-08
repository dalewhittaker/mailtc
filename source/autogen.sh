#!/bin/sh

LIBTOOLIZE_FLAGS="--force --automake --copy"
ACLOCAL_FLAGS="-I autotools"
AUTOMAKE_FLAGS="--foreign --add-missing --copy"
rm -rf ./autom4te.cache
mkdir -p autotools
touch README INSTALL

aclocal $ACLOCAL_FLAGS || exit $?
libtoolize $LIBTOOLIZE_FLAGS || exit $?
autoheader || exit $?
automake $AUTOMAKE_FLAGS || exit $?
autoconf || exit $?

./configure "$@" || exit $?
