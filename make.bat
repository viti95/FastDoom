@echo off

echo ==============================================
echo = FastDOOM make tool for DOS                 =
echo = Usage: make.bat compile_mode target        =
echo = compile_mode: clean (full clean & build)   =
echo = compile_mode: build (just build)           =
echo = Check the README for possible targets      =
echo ==============================================
echo.

if "%1"=="" GOTO missing_parameters
if "%2"=="" GOTO missing_parameters

if "%2"=="13H" GOTO mode_13h
if "%2"=="ATI" GOTO mode_ati
if "%2"=="BWC" GOTO mode_bwc
if "%2"=="C16" GOTO mode_c16
if "%2"=="CGA" GOTO mode_cga
if "%2"=="CVB" GOTO mode_cvb
if "%2"=="E"   GOTO mode_e
if "%2"=="E16" GOTO mode_e16
if "%2"=="HGC" GOTO mode_hgc
if "%2"=="PCP" GOTO mode_pcp
if "%2"=="T1"  GOTO mode_t1
if "%2"=="T12" GOTO mode_t12
if "%2"=="T25" GOTO mode_t25
if "%2"=="T43" GOTO mode_t43
if "%2"=="T50" GOTO mode_t50
if "%2"=="T52" GOTO mode_t52
if "%2"=="T86" GOTO mode_t86
if "%2"=="V2"  GOTO mode_v2
if "%2"=="V16" GOTO mode_v16
if "%2"=="VBD" GOTO mode_vbd
if "%2"=="VBR" GOTO mode_vbr
if "%2"=="Y"   GOTO mode_y
if "%2"=="MDA" GOTO mode_mda
if "%2"=="E80" GOTO mode_e80
if "%2"=="EW1" GOTO mode_ew1
if "%2"=="EGA" GOTO mode_ega
if "%2"=="CAH" GOTO mode_cah

:mode_13h
set base=fdoom13h.exe
set executable=fdoom13h.exe
set options=/dMODE_13H
goto compile_mode

:mode_ati
set base=fdoom13h.exe
set executable=fdoomati.exe
set options=/dMODE_ATI640
goto compile_mode

:mode_bwc
set base=fdoom13h.exe
set executable=fdoombwc.exe
set options=/dMODE_CGA_BW
goto compile_mode

:mode_c16
set base=fdoom13h.exe
set executable=fdoomc16.exe
set options=/dMODE_CGA16
goto compile_mode

:mode_cah
set base=fdoom13h.exe
set executable=fdoomcah.exe
set options=/dMODE_CGA_AFH
goto compile_mode

:mode_cga
set base=fdoom13h.exe
set executable=fdoomcga.exe
set options=/dMODE_CGA
goto compile_mode

:mode_cvb
set base=fdoom13h.exe
set executable=fdoomcvb.exe
set options=/dMODE_CVB
goto compile_mode

:mode_e
set base=fdoom13h.exe
set executable=fdoome.exe
set options=/dMODE_EGA640
goto compile_mode

:mode_e16
set base=fdoom13h.exe
set executable=fdoome16.exe
set options=/dMODE_EGA16
goto compile_mode

:mode_hgc
set base=fdoom13h.exe
set executable=fdoomhgc.exe
set options=/dMODE_HERC
goto compile_mode

:mode_pcp
set base=fdoom13h.exe
set executable=fdoompcp.exe
set options=/dMODE_PCP
goto compile_mode

:mode_t1
set base=fdoomtxt.exe
set executable=fdoomt1.exe
set options=/dMODE_T4025
goto compile_mode

:mode_t12
set base=fdoomtxt.exe
set executable=fdoomt12.exe
set options=/dMODE_T4050
goto compile_mode

:mode_t25
set base=fdoomtxt.exe
set executable=fdoomt25.exe
set options=/dMODE_T8025
goto compile_mode

:mode_t43
set base=fdoomtxt.exe
set executable=fdoomt43.exe
set options=/dMODE_T8043
goto compile_mode

:mode_t50
set base=fdoomtxt.exe
set executable=fdoomt50.exe
set options=/dMODE_T8050
goto compile_mode

:mode_t52
set base=fdoomtxt.exe
set executable=fdoomt52.exe
set options=/dMODE_T80100
goto compile_mode

:mode_t86
set base=fdoomtxt.exe
set executable=fdoomt86.exe
set options=/dMODE_T8086
goto compile_mode

:mode_v2
set base=fdoom13h.exe
set executable=fdoomv2.exe
set options=/dMODE_V2
goto compile_mode

:mode_e80
set base=fdoom13h.exe
set executable=fdoome80.exe
set options=/dMODE_EGA80
goto compile_mode

:mode_ew1
set base=fdoom13h.exe
set executable=fdoomew1.exe
set options=/dMODE_EGAW1
goto compile_mode

:mode_ega
set base=fdoom13h.exe
set executable=fdoomega.exe
set options=/dMODE_EGA
goto compile_mode

:mode_v16
set base=fdoom13h.exe
set executable=fdoomv16.exe
set options=/dMODE_VGA16
goto compile_mode

:mode_vbd
set base=fdoomvbd.exe
set executable=fdoomvbd.exe
set options=/dMODE_VBE2_DIRECT
goto compile_mode

:mode_vbr
set base=fdoomvbe.exe
set executable=fdoomvbr.exe
set options=/dMODE_VBE2
goto compile_mode

:mode_y
set base=fdoomy.exe
set executable=fdoom.exe
set options=/dMODE_Y
goto compile_mode

:mode_mda
set base=fdoomtxt.exe
set executable=fdoommda.exe
set options=/dMODE_MDA
goto compile_mode

:compile_mode
if "%1"=="clean" GOTO clean
if "%1"=="build" GOTO build

:build
cd fastdoom
wmake %base% EXTERNOPT="%options%" 
copy %base% ..\%executable%
cd ..
sb -r %executable%
ss %executable% dos32a.d32
goto end

:clean
cd fastdoom
wmake clean
wmake %base% EXTERNOPT="%options%" 
copy %base% ..\%executable%
cd ..
sb -r %executable%
ss %executable% dos32a.d32
goto end

:missing_parameters
echo Wrong parameters
goto EOF

:end
echo RIP AND TEAR
goto EOF

:EOF
