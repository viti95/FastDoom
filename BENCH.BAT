@echo off

cls

echo.   
echo.   
echo.   ##########################################################################
echo.   #                                                                        #
echo.   #                           FASTDOOM BENCHMARK                           #
echo.   #                                                                        #
echo.   ##########################################################################

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
echo.
echo Executing benchmarks...
echo.

if "%5"=="advanced" goto do_advanced_bench

%2 -iwad %3 -benchmark file %4 bench\%1.bnc
goto benchmark_finished

:do_advanced_bench
%2 -iwad %3 -benchmark file %4 bench\%1.bnc -advanced
goto benchmark_finished_advanced

:error_parameters
echo.   #                                                                        #
echo.   #    Usage:  bench.bat [type] [exe] [iwad] [lmp] [option]                #
echo.   #                                                                        #
echo.   #   [type]:  [phils, quick, normal, arch, ...]                           #
echo.   #    [exe]:  [fdoom, fdoom13h, ...]                                      #
echo.   #   [iwad]:  [doom1.wad, doomu.wad, doom2.wad, ...]                      #
echo.   #    [lmp]:  [demo1, demo2, ...]                                         #
echo.   # [option]:  [-empty-,advanced]                                          #
echo.   #                                                                        #
echo.   #  Examples:                                                             #
echo.   #                                                                        #
echo.   #       bench.bat phils fdoom doom1.wad demo3                            #
echo.   #       bench.bat normal fdoom13h doom2.wad demo1 advanced               #
echo.   #                                                                        #
echo.   ##########################################################################
echo.
echo.   
goto EOF

:benchmark_finished
cls
echo Benchmark finished
echo Results stored in file bench.csv
echo.
goto EOF

:benchmark_finished_advanced
cls
echo Benchmark finished
echo Results stored in file bench.csv
echo Frametimes stored in file ftime.csv
echo.
goto EOF

:EOF
