# MaTSa
This is the repository of MaTSa (aka Managed Thread Sanitizer) (formerly JTSan).

To build, use the build script (you will need an existing java 17+ build):

`./build_matsa.py (add -h to see options)`

Note: You need to have all the requirements to build OpenJDK, as mentioned [here](https://htmlpreview.github.io/?https://raw.githubusercontent.com/openjdk/jdk/master/doc/building.html#boot-jdk-requirements).

By the default, the build will be at the following directory: build/linux-x86_64-server-release/jdk/bin 

You can either run your programs from within this directory or create a symbolic link to the java executable or a variable pointing to that location.

For a variable, you need something similar to this `export MATSA_HOME=build/linux-x86_64-server-release/jdk/bin`

Running programs with the tool only requires the MaTSa flag.

e.g `$MATSA_HOME/java -XX:+MaTSa program.java`

If you wish to enable C1 instrumentation you need additionally pass the experimental flag.

e.g `$MATSA_HOME/java -XX:+MaTSa -XX:+MaTSaExperimental program.java`

In some rare cases MaTSa might failed to produce a complete report, due to previous stacks being recycled.
In those cases, you may increase the history size by setting the flag `MaTSaHistorySize`. The range is **1 to 16**, the higher the value the higher the memory consumption.

Default value is **4**.

e.g `$MATSA_HOME/java -XX:+MaTSa -XX:+MaTSaExperimental -XX:+MaTSaHistorySize=10 program.java`

**MaTSa currently only works for x86 and linux.**
