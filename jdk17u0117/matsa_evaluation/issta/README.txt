This directory contains the data we used the evaluation. 
All the data was obtained by running the dacapo benchmark suite.

jtsan is MaTSa.

jtsan_gc_diff.tar.gz <- Showcases the difference in reported races with or without garbage collection (note that only the benchmarks with at least one race are included and times can be ignored)
jtsan_mem.tar.gz <- the peak memory usage per benchmark by MaTSa
jtsan_run_fixed_traces.tar.gz <- Due to a bug when running evaluation some traces are missing, this has been fixed before submission and these were used to check the validity of the tool (ignore times, as they are taken from a different,slower computer).
jtsan_runs.tar.gz <- 5 runs of the benchmarks with MaTSa, we took the times from this.
llvm_java_tsan_runs.tar.gz <- 5 runs of the benchmarks with LLVM TSan Java.
llvm_tsan_mem_peak.tar.gz <- peak memory usage of LLVM TSan Java.
vanilla_stuff.tar.gz <- Contains vanilla times and memory usage for both tools and jdks used.
