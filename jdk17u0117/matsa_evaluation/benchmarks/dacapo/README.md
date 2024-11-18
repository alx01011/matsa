# DaCapo Benchmarks

## Running the benchmarks

You can run the benchmarks using the provided `dacapo.sh` script. The script will prompt you to download the DaCapo benchmarks if you haven't already.

The script can be run in the following major modes:
- `./dacapo.sh -a` to run all the selected benchmarks
- `./dacapo.sh <benchmark>` to run a specific benchmark
- `./dacapo.sh -h` will show additional options

Note that you may need to modify some of the script's variables to match your system configuration.

## Reading the results

The script will output the results in the current directory in two files (per benchmark):
- `<benchmark>_out.txt` contains the stdout of the benchmark
- `<benchmark>_err.txt` contains the stderr of the benchmark

MaTSa's output will be in the `.err` file and might contain several warnings along with their stack traces.

If you want to run with the `LLVM Java TSan` project, you may want to filter out reports that refer to the same code location, or include redundant information. This may be done by using the `filter.sh <file>` script. The script will filter out the reports and output the results in stdout.