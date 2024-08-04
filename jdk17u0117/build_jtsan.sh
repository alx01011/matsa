#!/bin/bash

# Usage:
# ./build_jtsan.sh -fd (fast debug build)
# ./build_jtsan.sh -sd (slow debug build)
# ./build_jtsan.sh -r (release build) (default)

CONFIGURE_FLAGS='--disable-warnings-as-errors '
JTSAN_BUILD_CXXFLAGS='--with-extra-cxxflags=-DINCLUDE_JTSAN'
CONF='linux-x86_64-server-release'

# check for debug builds
if [ "$1" == "-fd" ]; then
    CONFIGURE_FLAGS='--disable-warnings-as-errors --with-debug-level=fastdebug'
    CONF='linux-x86_64-server-fastdebug'
    shift
elif [ "$1" == "-sd" ]; then
    CONFIGURE_FLAGS='--disable-warnings-as-errors --with-debug-level=slowdebug'
    CONF='linux-x86_64-server-slowdebug'
    shift
fi

if [ ! -f configure ]; then
    echo "JTSan BUild Error: configure file not found"
    exit 1
fi

/bin/bash configure $CONFIGURE_FLAGS $JTSAN_BUILD_CXXFLAGS

make CONF=$CONF images

