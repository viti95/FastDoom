@echo off

if "%1"=="" GOTO missing_parameters
if "%2"=="" GOTO missing_parameters

if "%2"=="fdoom13h.exe" GOTO mode_13h
if "%2"=="fdoomati.exe" GOTO mode_ati
if "%2"=="fdoombwc.exe" GOTO mode_bwc
if "%2"=="fdoomc16.exe" GOTO mode_c16
if "%2"=="fdoomcga.exe" GOTO mode_cga
if "%2"=="fdoomcvb.exe" GOTO mode_cvb
if "%2"=="fdoome.exe"   GOTO mode_e
if "%2"=="fdoomhgc.exe" GOTO mode_hgc
if "%2"=="fdoompcp.exe" GOTO mode_pcp
if "%2"=="fdoomt1.exe"  GOTO mode_t1
if "%2"=="fdoomt12.exe" GOTO mode_t12
if "%2"=="fdoomt25.exe" GOTO mode_t25
if "%2"=="fdoomt43.exe" GOTO mode_t43
if "%2"=="fdoomt50.exe" GOTO mode_t50
if "%2"=="fdoomt86.exe" GOTO mode_t86
if "%2"=="fdoomv2.exe"  GOTO mode_v2
if "%2"=="fdoomv16.exe" GOTO mode_v16
if "%2"=="fdoomvbd.exe" GOTO mode_vbd
if "%2"=="fdoomvbr.exe" GOTO mode_vbr
if "%2"=="fdoom.exe"    GOTO mode_y
if "%2"=="fdoommda.exe" GOTO mode_mda
if "%2"=="fdoome80.exe" GOTO mode_e80
if "%2"=="fdoomew1.exe" GOTO mode_ew1
if "%2"=="fdoomega.exe" GOTO mode_ega
if "%2"=="fdoomcah.exe" GOTO mode_cah
if "%2"=="fdoom512.exe" GOTO mode_512

:mode_13h
set base=fdoom.exe
set executable=fdoom13h.exe
set options=-dMODE_13H
goto compile

:mode_ati
set base=fdoom.exe
set executable=fdoomati.exe
set options=-dMODE_ATI640
goto compile

:mode_bwc
set base=fdoom.exe
set executable=fdoombwc.exe
set options=-dMODE_CGA_BW
goto compile

:mode_c16
set base=fdoom.exe
set executable=fdoomc16.exe
set options=-dMODE_CGA16
goto compile

:mode_cah
set base=fdoom.exe
set executable=fdoomcah.exe
set options=-dMODE_CGA_AFH
goto compile

:mode_512
set base=fdoom.exe
set executable=fdoom512.exe
set options=-dMODE_CGA512
goto compile

:mode_cga
set base=fdoom.exe
set executable=fdoomcga.exe
set options=-dMODE_CGA
goto compile

:mode_cvb
set base=fdoom.exe
set executable=fdoomcvb.exe
set options=-dMODE_CVB
goto compile

:mode_e
set base=fdoom.exe
set executable=fdoome.exe
set options=-dMODE_EGA640
goto compile

:mode_hgc
set base=fdoom.exe
set executable=fdoomhgc.exe
set options=-dMODE_HERC
goto compile

:mode_pcp
set base=fdoom.exe
set executable=fdoompcp.exe
set options=-dMODE_PCP
goto compile

:mode_t1
set base=fdoom.exe
set executable=fdoomt1.exe
set options=-dMODE_T4025
goto compile

:mode_t12
set base=fdoom.exe
set executable=fdoomt12.exe
set options=-dMODE_T4050
goto compile

:mode_t25
set base=fdoom.exe
set executable=fdoomt25.exe
set options=-dMODE_T8025
goto compile

:mode_t43
set base=fdoom.exe
set executable=fdoomt43.exe
set options=-dMODE_T8043
goto compile

:mode_t50
set base=fdoom.exe
set executable=fdoomt50.exe
set options=-dMODE_T8050
goto compile

:mode_t86
set base=fdoom.exe
set executable=fdoomt86.exe
set options=-dMODE_T8086
goto compile

:mode_v2
set base=fdoom.exe
set executable=fdoomv2.exe
set options=-dMODE_V2
goto compile

:mode_e80
set base=fdoom.exe
set executable=fdoome80.exe
set options=-dMODE_EGA80
goto compile

:mode_ew1
set base=fdoom.exe
set executable=fdoomew1.exe
set options=-dMODE_EGAW1
goto compile

:mode_ega
set base=fdoom.exe
set executable=fdoomega.exe
set options=-dMODE_EGA
goto compile

:mode_v16
set base=fdoom.exe
set executable=fdoomv16.exe
set options=-dMODE_VGA16
goto compile

:mode_vbd
set base=fdoom.exe
set executable=fdoomvbd.exe
set options=-dMODE_VBE2_DIRECT
goto compile

:mode_vbr
set base=fdoom.exe
set executable=fdoomvbr.exe
set options=-dMODE_VBE2
goto compile

:mode_y
set base=fdoom.exe
set executable=fdoom.exe
set options=-dMODE_Y
goto compile

:mode_mda
set base=fdoom.exe
set executable=fdoommda.exe
set options=-dMODE_MDA
goto compile

:compile
if "%1"=="clean" GOTO clean
if "%1"=="build" GOTO build

:build
cd fastdoom
wmake %base% EXTERNOPT="%options%"
copy %base% ..\%executable%
cd ..
goto end

:clean
cd fastdoom
wmake clean
wmake %base% EXTERNOPT="%options%"
copy %base% ..\%executable%
cd ..
goto end

:missing_parameters
echo Wrong parameters
goto EOF

:end
echo RIP AND TEAR
goto EOF

:EOF
