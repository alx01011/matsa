#!/bin/bash

# Usage:
# ./build_matsa.sh -fd --boot-jvm <JVM17> (fast debug build)
# ./build_matsa.sh -sd --boot-jvm <JVM17> (slow debug build)
# ./build_matsa.sh --boot-jvm <JVM17> (release build) (default)

CONFIGURE_FLAGS='--disable-warnings-as-errors '
JTSAN_BUILD_CXXFLAGS='--with-extra-cxxflags=-DINCLUDE_MATSA'
BOOT_JVM=''
CONF='linux-x86_64-server-release'

# usage
if [ "$1" == "-h" ]; then
    echo "Usage: "
    echo "./build_matsa.sh -fd --boot-jvm \$JVM17 (fast debug build)"
    echo "./build_matsa.sh -sd --boot-jvm \$JVM17 (slow debug build)"
    echo "./build_matsa.sh --boot-jvm \$JVM17 (release build) (default)"
    exit 0
fi

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

# check for boot jvm
if [ "$1" == "--boot-jvm" ]; then
    BOOT_JVM=$2
    shift
fi

# check if boot-jvm has been set so we can include in flags
if [ -n "$BOOT_JVM" ]; then
    CONFIGURE_FLAGS="$CONFIGURE_FLAGS --with-boot-jdk=$BOOT_JVM"
fi

if [ ! -f configure ]; then
    echo "JTSan BUild Error: configure file not found"
    exit 1
fi

/bin/bash configure $CONFIGURE_FLAGS $JTSAN_BUILD_CXXFLAGS

make CONF=$CONF clean
make CONF=$CONF images

