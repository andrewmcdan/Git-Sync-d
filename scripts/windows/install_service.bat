@echo off
set SERVICE_NAME=GitSyncD
set SERVICE_DESC="Git Sync'd service"
set BIN_PATH="%~dp0Git-Sync-d.exe --start"

sc.exe create %SERVICE_NAME% binPath= %BIN_PATH% start= auto
sc.exe description %SERVICE_NAME% %SERVICE_DESC%
echo Service %SERVICE_NAME% installed.
