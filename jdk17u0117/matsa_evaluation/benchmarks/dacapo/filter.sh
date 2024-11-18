if [ $# -ne 1 ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

FILE=$1

awk '/SUMMARY:/ {
    if (!seen[$0]++) {  # Check if the "SUMMARY:" line is already seen
        if (block) print block; 
        block = $0;
        flag = 1; 
        next;
    } else {
        flag = 0;  # Disable flag if "SUMMARY:" line is a duplicate
    }
}
flag {block = block "\n" $0}  # Append subsequent lines to the block
/^$/ {flag = 0}  # Reset flag on an empty line
END {
    if (block) print block;  # Print the last block if it exists
}' "$FILE"
