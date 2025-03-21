#!/bin/bash

# Runs all tests in folders race and no_race

# Usage: test.sh [options]
# Options:
#   -h, Display this help and exit
#   -fd, Run with fast debug build
#   -sd, Run with slow debug build
#   -s, Adds additional switches to java command
#   -a, Run all benchmarks

# Parse options
#JAVA_OPTS="-Xint -XX:-RewriteFrequentPairs -XX:-UseCompressedOops -XX:-UseCompressedClassPointers -XX:+UseParallelGC -XX:+JTSan"
JAVA_OPTS="-XX:+MaTSa"
JAVA_OPTS_TSAN="-XX:-UseCompressedOops -XX:-UseCompressedClassPointers -XX:+ThreadSanitizer"
# Set up environment
BUILD=../../../build/linux-x86_64-server-release/jdk/bin/
BUILD_TSAN=/spare/aantonak/repos/jdk-tsan/build/linux-x86_64-server-release/images/jdk/bin/

if [ "$1" == "-h" ]; then
    echo "Usage: test.sh [options]"
    echo "Options:"
    echo "  -h, Display this help and exit"
    echo "  -fd, Run with fast debug build"
    echo "  -sd, Run with slow debug build"
    echo "  -s, Adds additional switches to java command"
    echo "  -a, Run all benchmarks"
    echo "  -p, Run under perf"
    echo "  -tsan, running using llvm java tsan"
    exit 0
fi

if [ "$1" == "-fd" ]; then
    BUILD=../../../build/linux-x86_64-server-fastdebug/jdk/bin/
    shift
fi

if [ "$1" == "-sd" ]; then
    BUILD=../../../build/linux-x86_64-server-slowdebug/jdk/bin/
    shift
fi

if [ "$1" == "-tsan" ]; then
    BUILD=$BUILD_TSAN
    JAVA_OPTS=$JAVA_OPTS_TSAN
    shift
fi

if [ "$1" == "-int" ]; then
    JAVA_OPTS="-XX:-UseCompressedOops -XX:-UseCompressedClassPointers -Xint"
    shift
fi

if [ "$1" == "-p" ]; then
    BUILD="perf record $BUILD"
    shift
fi

if [ "$1" == "-s" ]; then
    JAVA_OPTS="$JAVA_OPTS $2"
    shift
    shift
fi
# h2o,spring dnf
# trade* crash
# h2,tomcat,cassandra oom
# kafka way too many error (in the benchmark itself)
BENCHMARKS="avrora batik biojava eclipse fop graphchi jme jython luindex lusearch pmd sunflow xalan zxing"

#BENCHMARKS="lusearch pmd sunflow tomcat tradebeans tradesoap xalan zxing"

run_benchmark() {
    local benchmark=$1
    local opts=$JAVA_OPTS
    if [ "$benchmark" == "cassandra" ]; then
        opts="$opts -Djava.security.manager=allow"
    fi

    echo "Starting benchmark: $benchmark"

    $BUILD/java -jar $opts dacapo-23.11-chopin.jar -p $benchmark > "${benchmark}_out.txt" 2> "${benchmark}_error.txt"

    echo "Finished benchmark: $benchmark"
}

# do you have dacapo?
if [ ! -f dacapo-23.11-chopin.jar ]; then
    echo "DaCapo jar not found."
    echo "Do you want to download it? (y/n)"
    read answer

    if [ "$answer" == "y" ]; then
        wget https://download.dacapobench.org/chopin/dacapo-23.11-chopin.zip
        unzip dacapo-23.11-chopin.zip
    else
        echo "Exiting (got $answer)."
        exit 1
    fi
fi


if [ "$1" == "-a" ]; then
    for benchmark in $BENCHMARKS; do
        run_benchmark $benchmark
    done
else
    run_benchmark $1
    wait
fi

