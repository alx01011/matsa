# MaTSa
This is the repository of MaTSa (aka Managed Thread Sanitizer) (formerly JTSan).

You can build the tool by running `./build_matsa.sh --boot-jvm <JDK17>`

To run any app with the tool you only need to specify the `-XX:+MaTSa` switch. e.g `<path_to_build>/java -XX:+MaTSa Main.java`.
Warnings will be generated at runtime and are printed on `stderr`.


The tool is currently implemented only for the interpreter so expect big performance drops.
