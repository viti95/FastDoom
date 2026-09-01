@echo off
if not exist FDSTART.EXE goto missing
FDSTART.EXE
goto end
:missing
echo FDSTART.EXE not found.
:end
