# Runs all tests in folders race and no_race

# Usage: test.sh [options]
# Options:
#   -h, Display this help and exit
#   -fd, Run with fast debug build
#   -sd, Run with slow debug build
#   -s, Adds additional switches to java command

# Parse options
JAVA_OPTS="-XX:-UseCompressedOops -XX:-UseCompressedClassPointers -Xint -XX:+UseParallelGC -XX:+JTSAN"

while getopts "hs:" opt; do
    case $opt in
        h)
            echo "Usage: test.sh [options]"
            echo "Options:"
            echo "  -h, Display this help and exit"
            echo "  -s, Adds additional switches to java command"
            exit 0
            ;;
        s)
            JAVA_OPTS="$JAVA_OPTS $OPTARG"
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            ;;
    esac
done

# Set up environment
BUILD=../build/linux-x86_64-server-release/jdk/bin/

#check for debug
if [ "$1" == "-fd" ]; then
    BUILD=../build/linux-x86_64-server-fastdebug/jdk/bin/
    shift
elif [ "$1" == "-sd" ]; then
    BUILD=../build/linux-x86_64-server-slowdebug/jdk/bin/
    shift
fi

# Run non racy tests (in no_race dir)
echo -e "\e[33mRunning non racy tests\e[0m"

for i in `ls ./no_race/*.java`; do
    echo -e "\e[32mRunning $i\e[0m"
    $BUILD/javac $i
    $BUILD/java $JAVA_OPTS `basename $i .java`

    rm -f `basename $i .java`.class
    # some spacing
    echo ""
done

# Run racy
echo -e "\e[33mRunning racy tests\e[0m"

for i in `ls ./race/*.java`; do
    echo -e "\e[32mRunning $i\e[0m"
    $BUILD/javac $i
    $BUILD/java $JAVA_OPTS `basename $i .java`

    # cleanup
    rm -f `basename $i .java`.class
    # some spacing
    echo ""
done