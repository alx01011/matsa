#!/usr/bin/env python3

import os
import sys
import platform
import subprocess

SUPPORTED_OS = ['Linux']

def usage():
    print("Usage: build_matsa.py [options]")
    print("Options:")
    print("  -h, --help             Show this help message")
    print("  -fd, --fast-debug      Build with fast debug configuration")
    print("  -sd,  --slow-debug     Build with slow debug configuration")
    print("  --with-boot-jdk=<path> Specify the boot JDK path")
    sys.exit(0)

def die(msg):
    print(f"Error: {msg}")
    sys.exit(1)

# checks if the cpu supports AVX2 and returns the arg or an empty string
def get_avx2_arg():
    try:
        with open('/proc/cpuinfo', 'r') as f:
            cpuinfo = f.read()
        return '-mavx2' if 'avx2' in cpuinfo.lower() else ''
    except FileNotFoundError:
        die("Could not read /proc/cpuinfo. Make sure you are running on a Linux system.")

def check_os():
    op_sys = platform.system()
    # see if we support the current OS
    return op_sys.lower() in  [os.lower() for os in SUPPORTED_OS]

def get_wsl_args():
    wsl_version = platform.uname().release
    if wsl_version.endswith("-Microsoft") or  wsl_version.endswith("microsoft-standard-WSL2"):
        return '--build=x86_64-unknown-linux-gnu'
    return ''

def configure_openjdk(boot_jdk, level):
    boot_jdk_cmd = '--with-boot-jdk=' + boot_jdk if boot_jdk else ''
    matsa_args = f'--with-extra-cxxflags=-DINCLUDE_MATSA {get_avx2_arg()}'
    configure_cmd = ['bash', 'configure', level, '--disable-warnings-as-errors', get_wsl_args(), boot_jdk_cmd, matsa_args]
    print(f"Configuring OpenJDK with command: {' '.join(configure_cmd)}")

    result = subprocess.run(configure_cmd, check=True)
    if result.returncode != 0:
        die("Configuration failed. Please check the output for errors.")
    print("OpenJDK configured successfully.")

def build_openjdk(configuration):
    # set environment variable for configuration
    os.environ['CONF'] = configuration
    make_cmd = ['make', 'clean']

    result = subprocess.run(make_cmd, check=True)
    if result.returncode != 0:
        die("Clean failed. Please check the output for errors.")
    
    make_cmd = ['make', 'images']

    print(f"Building OpenJDK with command: {' '.join(make_cmd)} and  configuration: {configuration}")

    result = subprocess.run(make_cmd, check=True)
    if result.returncode != 0:
        die("Build failed. Please check the output for errors.")


def main():
    # fail if we don't have the right OS before parsing
    if not check_os():
        die(f"Unsupported OS: {platform.system()}. Supported OS: {', '.join(SUPPORTED_OS)}")
    
    # parse command line arguments
    args = sys.argv[1:]

    boot_jdk = None
    # defaults to release
    level = '--with-debug-level=release'
    configuration = 'linux-x86_64-server-release'

    for arg in args:
        if arg in ('-h', '--help'):
            usage()
        elif arg in ('-fd', '--fast-debug'):
            print("Building with fast debug configuration...")
            level = '--with-debug-level=fastdebug'
            configuration = 'linux-x86_64-server-fastdebug'
        elif arg in ('-sd', '--slow-debug'):
            print("Building with slow debug configuration...")
            level = '--with-debug-level=slowdebug'
            configuration = 'linux-x86_64-server-slowdebug'
        elif arg.startswith('--with-boot-jdk='):
            boot_jdk = arg.split('=')[1]
            if not boot_jdk:
                die("Boot JDK path must be specified after --boot-jdk=")
            print(f"Using boot JDK: {boot_jdk}")
            # Add logic to use the specified boot JDK
        else:
            die(f"Unknown option: {arg}")
    
    configure_openjdk(boot_jdk, level)
    build_openjdk(configuration)
    print("Build process completed successfully.")

if __name__ == '__main__':
    main()