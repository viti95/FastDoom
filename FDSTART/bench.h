/*
 * BENCH.H - FastDoom text mode launcher, the benchmark launcher
 */

#ifndef BENCH_H
#define BENCH_H

/*
 * Benchmark launcher: the user picks the IWAD (-iwad), a benchmark
 * file (BENCH\*.BNC), the demo and the executable, and it is run
 * with -benchmark file (and -advanced for the frametimes loop).
 * Returns 1 if the launcher should quit, 0 to go back to the main
 * menu.
 */
int bench_menu(void);

#endif /* BENCH_H */
