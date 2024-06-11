@echo off

if "%1"=="" goto all

sb /R %1 >> stub.log
ss %1 dos32a.d32 >> stub.log

exit

:all

for %%f in (fdoom*.exe) do sb /R %%f >> stub.log
for %%f in (fdoom*.exe) do ss %%f dos32a.d32 >> stub.log
for %%f in (fdm*.exe) do sb /R %%f >> stub.log
for %%f in (fdm*.exe) do ss %%f dos32a.d32 >> stub.log

exit

