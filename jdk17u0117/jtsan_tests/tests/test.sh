# Runs all tests in folders race and no_race

# Usage: test.sh [options]
# Options:
#   -h, Display this help and exit
#   -fd, Run with fast debug build
#   -sd, Run with slow debug build
#   -s, Adds additional switches to java command

# Parse options
JAVA_OPTS="-XX:+JTSan"
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

# Color codes for output
YELLOW='\e[33m'
GREEN='\e[32m'
RESET='\e[0m'
RED='\e[31m'
BLUE='\e[34m'

# Function to compile Java files
compile_java_files() {
    local dir=$1
    echo -e "${YELLOW}Compiling $dir tests${RESET}"
    mkdir -p ./class_files
    for i in ./$dir/*.java; do
        [ -e "$i" ] || continue
        base_name=$(basename "$i" .java)
        echo -e "Compiling $base_name"
        if ! $BUILD/javac -d ./class_files "$i"; then
            echo -e "${RED}Compilation failed for $base_name${RESET}"
            return 1
        fi
    done
    echo ""
}

run_java_files() {
    local dir=$1
    echo -e "${YELLOW}Running tests in $dir${RESET}"
    local non_racy_pass=0
    local non_racy_total=0
    local racy_pass=0
    local racy_total=0
    local other_total=0

    for i in ./$dir/*.java; do
        [ -e "$i" ] || continue
        base_name=$(basename "$i" .java)
        echo -n "$base_name : "
        
        $BUILD/java -cp ./class_files $JAVA_OPTS "$base_name" > .test.out.tmp 2>&1
        
        if [[ "$base_name" =~ ^nr_ || "$base_name" =~ ^NonRacy ]]; then
            if grep -q "0 warnings" .test.out.tmp; then
                echo -e "${GREEN}PASS${RESET}"
                ((non_racy_pass++))
            else
                echo -e "${RED}FAIL${RESET}"
                echo "Output:"
                cat .test.out.tmp
            fi
        elif [[ "$base_name" =~ ^r_ || "$base_name" =~ ^Racy ]]; then
            if grep -q "0 warning" .test.out.tmp; then
                echo -e "${RED}FAIL${RESET}"
                echo "Output:"
                cat .test.out.tmp
            else
                echo -e "${GREEN}PASS${RESET}"
            fi
        else
            echo -e "${YELLOW}Unknown test kind: $base_name${RESET}"
            echo "Output:"
            cat .test.out.tmp
        fi
    done
    echo ""
}


# Function to process a test directory
process_test_directory() {
    local dir=$1
    if compile_java_files "$dir"; then
        run_java_files "$dir"
    else
        echo -e "${RED}Skipping test execution for $dir due to compilation errors${RESET}"
    fi
}

# Main function
main() {
    echo -e "${YELLOW}Test execution started at $(date)${RESET}"
    process_test_directory "no_race"
    process_test_directory "race"
    process_test_directory "tsan_nr"
    echo -e "${YELLOW}Test execution completed at $(date)${RESET}"
}

# Run the main function
main

# Clean up
echo -e "${YELLOW}Cleaning up${RESET}"
rm -rf ./class_files
rm -f .test.out.tmp
