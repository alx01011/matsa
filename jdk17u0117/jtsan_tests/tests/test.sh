# Runs all tests in folders race and no_race

# Usage: test.sh [options]
# Options:
#   -h, Display this help and exit
#   -fd, Run with fast debug build
#   -sd, Run with slow debug build
#   -s, Adds additional switches to java command

# Parse options
JAVA_OPTS="-XX:-UseCompressedOops -XX:-UseCompressedClassPointers -Xint -XX:+UseParallelGC -XX:+JTSAN"
# Set up environment
BUILD=../build/linux-x86_64-server-release/jdk/bin/

while getopts "hs:" opt; do
    case $opt in
        h)
            echo "Usage: test.sh [options]"
            echo "Options:"
            echo "  -h, Display this help and exit"
            echo "  -fd, Run with fast debug build"
            echo "  -sd, Run with slow debug build"
            echo "  -s, Adds additional switches to java command"
            exit 0
            ;;
        s)
            JAVA_OPTS="$JAVA_OPTS $OPTARG"
            ;;
        fd)
            BUILD=../build/linux-x86_64-server-fastdebug/jdk/bin/
            ;;
        sd)
            BUILD=../build/linux-x86_64-server-slowdebug/jdk/bin/
            ;;
        \?)
            echo "Invalid option: $OPTARG" 1>&2
            exit 1
            ;;
    esac
done


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