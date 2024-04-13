#!/bin/bash

# Usage: 
# ./test.sh -d (debug build) File.java

# Set up environment

BUILD=../build/linux-x86_64-server-release/jdk/bin/


#check for debug
if [ "$1" == "-d" ]; then
    BUILD=../build/linux-x86_64-server-fastdebug/jdk/bin/
    shift
fi


FILE=$1

# Compile the file
$BUILD/javac $FILE


ARGS='-XX:-UseCompressedOops -XX:-UseCompressedClassPointers -Xms100m -Xmx100g -Xint -XX:+UseParallelGC -XX:+JTSAN'

# Run
$BUILD/java $ARGS `basename $FILE .java`
 