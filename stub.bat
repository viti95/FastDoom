@echo off

if "%1"=="" goto all

sb /R %1
ss %1 dos32a.d32

exit

:all

for %%f in (fdoom*.exe) do sb /R %%f
for %%f in (fdoom*.exe) do ss %%f dos32a.d32
for %%f in (fdm*.exe) do sb /R %%f
for %%f in (fdm*.exe) do ss %%f dos32a.d32

exit

