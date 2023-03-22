@echo off

echo ################################################
echo #              FASTDOOM BENCHMARK              #
echo ################################################
echo # Usage:   bench.bat executable iwad demo      #
echo # Example: bench.bat fdoom.exe doomu.wad demo3 #
echo ################################################
echo.

If "%1"=="" goto error_parameters
If "%2"=="" goto error_parameters
If "%3"=="" goto error_parameters

echo detail;size;visplanes;sky;objects;transparent_columns;iwad;demo;gametics;realtics;fps > bench.csv

%1 -potato -size 12 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 11 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 10 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 9 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 8 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 7 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 6 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 5 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 4 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 3 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -low -size 12 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 11 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 10 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 9 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 8 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 7 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 6 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 5 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 4 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 3 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -high -size 12 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 11 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 10 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 9 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 8 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 7 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 6 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 5 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 4 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 3 -defVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -potato -size 12 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 11 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 10 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 9 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 8 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 7 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 6 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 5 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 4 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 3 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -low -size 12 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 11 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 10 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 9 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 8 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 7 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 6 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 5 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 4 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 3 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -high -size 12 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 11 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 10 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 9 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 8 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 7 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 6 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 5 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 4 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 3 -flatVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -potato -size 12 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 11 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 10 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 9 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 8 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 7 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 6 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 5 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 4 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 3 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -low -size 12 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 11 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 10 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 9 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 8 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 7 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 6 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 5 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 4 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 3 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -high -size 12 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 11 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 10 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 9 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 8 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 7 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 6 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 5 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 4 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 3 -flatterVisplanes -defSky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -potato -size 12 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 11 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 10 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 9 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 8 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 7 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 6 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 5 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 4 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 3 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -low -size 12 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 11 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 10 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 9 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 8 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 7 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 6 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 5 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 4 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 3 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -high -size 12 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 11 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 10 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 9 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 8 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 7 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 6 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 5 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 4 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 3 -defVisplanes -flatsky -far -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -potato -size 12 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 11 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 10 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 9 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 8 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 7 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 6 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 5 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 4 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 3 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -low -size 12 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 11 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 10 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 9 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 8 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 7 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 6 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 5 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 4 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 3 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -high -size 12 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 11 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 10 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 9 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 8 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 7 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 6 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 5 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 4 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 3 -defVisplanes -defSky -near -defShadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -potato -size 12 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 11 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 10 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 9 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 8 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 7 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 6 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 5 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 4 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 3 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -low -size 12 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 11 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 10 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 9 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 8 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 7 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 6 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 5 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 4 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 3 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -high -size 12 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 11 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 10 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 9 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 8 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 7 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 6 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 5 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 4 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 3 -defVisplanes -defSky -far -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -potato -size 12 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 11 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 10 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 9 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 8 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 7 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 6 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 5 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 4 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -potato -size 3 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -low -size 12 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 11 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 10 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 9 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 8 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 7 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 6 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 5 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 4 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -low -size 3 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -high -size 12 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 11 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 10 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 9 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 8 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 7 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 6 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 5 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 4 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -high -size 3 -defVisplanes -defSky -far -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv

goto benchmark_finished

:error_parameters
echo ERROR: Missing parameters
goto EOF

:benchmark_finished
echo Benchmark finished. Results stored in file bench.csv
goto EOF

:EOF