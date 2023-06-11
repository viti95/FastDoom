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

if exist bench.csv goto bypass
echo executable;arch;detail;size;visplanes;sky;objects;transparent_columns;iwad;demo;gametics;realtics;fps > bench.csv

:bypass
if "%1"=="phil" goto phil_bench
if "%1"=="quick" goto quick_bench
if "%1"=="normal" goto normal_bench
if "%1"=="full" goto full_bench
if "%1"=="arch" goto arch_bench

goto error_parameters

:full_bench
echo Executing full benchmarks...

%2 -potato -size 12 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -defSpan -flatsky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -defSpan -defSky -near -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -defSpan -defSky -far -flatInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -defSpan -defSky -far -flatsaturn -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -defSpan -defSky -far -saturn -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -potato -size 12 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 11 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 9 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 8 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 7 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 6 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 5 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 4 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 3 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 12 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 11 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 9 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 8 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 7 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 6 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 5 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 4 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 12 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 11 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 9 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 8 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 7 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 6 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 5 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 4 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 3 -defSpan -defSky -far -translucent -nofps -nomelt -iwad %3 -timedemo %4 -csv
goto benchmark_finished

:normal_bench
echo Executing standard benchmarks...
%2 -potato -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -potato -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -low -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv

%2 -high -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
goto benchmark_finished

:quick_bench
echo Executing quick benchmarks...
%2 -potato -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
goto benchmark_finished

:phil_bench
echo Executing PhilsComputerLab DOOM benchmarks...
%2 -high -size 12 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -low -size 3 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
goto benchmark_finished

:arch_bench
echo Executing Architectural benchmarks...
%2 -high -size 10 -386sx -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -386dx -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -cy386 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -cy486 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -i486 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -umc486 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -cy5x86 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -k5 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
%2 -high -size 10 -pentium -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
goto benchmark_finished

:error_parameters
echo #                                                                       #
echo #    Usage:  bench.bat [type] [exe] [iwad] [lmp]                        #
echo #                                                                       #
echo #   [type]:  [phil, quick, normal, full, arch]                          #
echo #    [exe]:  [fdoom.exe, fdoom13h.exe, ...]                             #
echo #   [iwad]:  [doom1.wad, doomu.wad, doom2.wad, ...]                     #
echo #    [lmp]:  [demo1, demo2, ...]                                        #
echo #                                                                       #
echo #  Example:  bench.bat phil fdoom.exe doom1.wad demo3                   #
echo #                                                                       #
echo #########################################################################
goto EOF

:benchmark_finished
cls
echo Benchmark finished. Results stored in file bench.csv
goto EOF

:EOF
