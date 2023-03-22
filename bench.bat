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

%1 -forcePQ -size 12 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv 
%1 -forcePQ -size 10 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 9 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv 
%1 -forcePQ -size 8 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 7 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 6 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 5 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 4 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 3 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceLQ -size 12 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 10 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 9 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 8 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 7 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 6 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 5 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 4 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 3 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceHQ -size 12 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 10 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 9 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 8 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 7 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 6 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 5 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 4 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 3 -normalsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forcePQ -size 12 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 10 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 9 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 8 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 7 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 6 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 5 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 4 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 3 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceLQ -size 12 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 10 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 9 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 8 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 7 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 6 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 5 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 4 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 3 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceHQ -size 12 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 10 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 9 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 8 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 7 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 6 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 5 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 4 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 3 -flatsurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forcePQ -size 12 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 10 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 9 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 8 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 7 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 6 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 5 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 4 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 3 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceLQ -size 12 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 10 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 9 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 8 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 7 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 6 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 5 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 4 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 3 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceHQ -size 12 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 10 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 9 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 8 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 7 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 6 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 5 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 4 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 3 -flattersurfaces -normalsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forcePQ -size 12 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 10 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 9 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 8 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 7 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 6 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 5 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 4 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 3 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceLQ -size 12 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 10 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 9 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 8 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 7 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 6 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 5 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 4 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 3 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceHQ -size 12 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 10 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 9 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 8 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 7 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 6 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 5 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 4 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 3 -normalsurfaces -flatsky -normalsprites -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forcePQ -size 12 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 10 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 9 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 8 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 7 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 6 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 5 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 4 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 3 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceLQ -size 12 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 10 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 9 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 8 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 7 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 6 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 5 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 4 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 3 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceHQ -size 12 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 10 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 9 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 8 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 7 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 6 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 5 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 4 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 3 -normalsurfaces -normalsky -near -normalshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forcePQ -size 12 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 10 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 9 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 8 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 7 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 6 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 5 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 4 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 3 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceLQ -size 12 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 10 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 9 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 8 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 7 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 6 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 5 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 4 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 3 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceHQ -size 12 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 10 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 9 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 8 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 7 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 6 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 5 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 4 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 3 -normalsurfaces -normalsky -normalsprites -flatshadows -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forcePQ -size 12 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 10 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 9 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 8 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 7 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 6 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 5 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 4 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forcePQ -size 3 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceLQ -size 12 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 10 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 9 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 8 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 7 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 6 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 5 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 4 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceLQ -size 3 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv

%1 -forceHQ -size 12 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 10 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 9 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 8 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 7 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 6 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 5 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 4 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv
%1 -forceHQ -size 3 -normalsurfaces -normalsky -normalsprites -saturn -nofps -nomelt -iwad %2 -timedemo %3 -csv

goto benchmark_finished

:error_parameters
echo ERROR: Missing parameters
goto EOF

:benchmark_finished
echo Benchmark finished. Results in file bench.csv
goto EOF

:EOF