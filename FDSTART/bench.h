/*
 * BENCH.H - FastDoom text mode launcher, the benchmark launcher
 */

#ifndef BENCH_H
#define BENCH_H

/*
 * Benchmark launcher: the user picks the benchmark type (multiple
 * benchmarks, -benchmark file, or a single benchmark,
 * -benchmark single), the IWAD (-iwad), the benchmark file
 * (BENCH\*.BNC, multiple benchmarks only), the demo and the
 * executable, and it is run with -benchmark file or -benchmark
 * single (and -advanced for the frametimes loop). Returns 1 if
 * the launcher should quit, 0 to go back to the main menu.
 */
int bench_menu(void);

#endif /* BENCH_H */
