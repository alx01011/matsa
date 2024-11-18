#!/usr/bin/env python3

import os
from collections import defaultdict
import sys

def usage():
    print("./filter.py <file_name>")

def read_and_filter_file(name):
    summ_dict = defaultdict(int)
    in_block = False
    block = ''

    with open(name, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if 'WARNING: ThreadSanitizer: data race (pid=10350)' in line:
                in_block = True
            elif 'Location is' in line:
                in_block = False
            elif 'SUMMARY' in line:
                if line not in summ_dict:
                    block += line
                    block += '==================\n' 
                    print(block)

                    summ_dict[line] = 1

                block = ''
            
            if in_block:
                block += line

def main():
    if len(sys.argv) < 2:
        usage()
        sys.exit(1)
    read_and_filter_file(sys.argv[1])

if __name__ == "__main__":
    main()