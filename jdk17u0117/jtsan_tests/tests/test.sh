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
BUILD=../../build/linux-x86_64-server-release/jdk/bin/

if [ "$1" == "-h" ]; then
    echo "Usage: test.sh [options]"
    echo "Options:"
    echo "  -h, Display this help and exit"
    echo "  -fd, Run with fast debug build"
    echo "  -sd, Run with slow debug build"
    echo "  -s, Adds additional switches to java command"
    exit 0
fi

if [ "$1" == "-fd" ]; then
    BUILD=../../build/linux-x86_64-server-fastdebug/jdk/bin/
    shift
fi

if [ "$1" == "-sd" ]; then
    BUILD=../../build/linux-x86_64-server-slowdebug/jdk/bin/
    shift
fi

if [ "$1" == "-s" ]; then
    JAVA_OPTS="$JAVA_OPTS $2"
    shift
    shift
fi

# create class output dir if it doesn't exist
mkdir -p ./class_files


# Run non racy tests (in no_race dir)
echo -e "\e[33mRunning non racy tests\e[0m"

for i in `ls ./no_race/*.java`; do
    # get basename
    base_name=$(basename $i .java)
    echo -e "\e[32mRunning $base_name\e[0m"

    # Compile Java file to the class_files directory
    $BUILD/javac -d ./class_files $i
    $BUILD/java -cp ./class_files $base_name $JAVA_OPTS

    # Some spacing
    echo ""
done


# Run racy
echo -e "\e[33mRunning racy tests\e[0m"

for i in `ls ./race/*.java`; do
    # get basename
    base_name=$(basename $i .java)
    echo -e "\e[32mRunning $base_name\e[0m"

    # Compile Java file to the class_files directory
    $BUILD/javac -d ./class_files $i
    $BUILD/java -cp ./class_files $base_name $JAVA_OPTS

    # Some spacing
    echo ""
done

# Clean up
echo -e "\e[33mCleaning up\e[0m"
rm -rf ./class_files