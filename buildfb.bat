@echo off

.\fbasic\fbc -arch 386 .\fbench\fbench.bas
copy /Y .\fbench\fbench.exe fbench.exe
