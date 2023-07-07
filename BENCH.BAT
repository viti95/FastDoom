@echo off

cls

echo #########################################################################
echo #                                                                       #
echo #                         FASTDOOM BENCHMARK                            #
echo #                                                                       #
echo #########################################################################

If "%1"=="" goto error_parameters
If "%2"=="" goto error_parameters
If "%3"=="" goto error_parameters
If "%4"=="" goto error_parameters

if "%1"=="phils" goto do_bench
if "%1"=="quick" goto do_bench
if "%1"=="normal" goto do_bench
if "%1"=="arch" goto do_bench

goto error_parameters

:do_bench
echo Executing benchmarks...
%2 -iwad %3 -benchmark %1 %4
goto benchmark_finished

:error_parameters
echo #                                                                       #
echo #    Usage:  bench.bat [type] [exe] [iwad] [lmp]                        #
echo #                                                                       #
echo #   [type]:  [phils, quick, normal, arch]                               #
echo #    [exe]:  [fdoom.exe, fdoom13h.exe, ...]                             #
echo #   [iwad]:  [doom1.wad, doomu.wad, doom2.wad, ...]                     #
echo #    [lmp]:  [demo1, demo2, ...]                                        #
echo #                                                                       #
echo #  Example:  bench.bat phils fdoom.exe doom1.wad demo3                  #
echo #                                                                       #
echo #########################################################################
goto EOF

:benchmark_finished
cls
echo Benchmark finished. Results stored in file bench.csv
goto EOF

:EOF
