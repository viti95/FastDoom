@echo off
for %%f in (fdoom*.exe) do sb /R %%f
for %%f in (fdoom*.exe) do ss %%f dos32a.d32
for %%f in (fdm*.exe) do sb /R %%f
for %%f in (fdm*.exe) do ss %%f dos32a.d32
exit
