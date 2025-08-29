# Git-Sync'd
Git client for syncing individual files to Git repos. 

## The Idea
So, if you have several files that you want to sync to a repo, but don't want to init the whole folder that the file exists in, or maybe you want to have a repo that has all the .gitignore files, this tool will sync individual files to a repo. 

Let's say you have several repos and each one has a .gitignore file that you want to collect into one repo. This tool can sync all these .gitignore files into that one repo and whenever it does so, it uses the git commit message from the git repo the .gitignore exists in.

Or maybe you have nixOS configs, docker files, or .env files for node.js projects that you want to sync between machines, but Google Drive and others like it are too heavy or otherwise ill-suited to the task. Enter Git-Sync'd.

Once you select a file to be sync'd, the app will search for it belonging to a git repo. If it does, Git-Sync'd will ask if you want to monitor that repo for changes and update on changes to that repo. Otherwise, it will ask if the user wants to update on changes or on an interval. 

If Git-Sync'd is monitoring a file that is part of an exiting repo, whenever that file is updated in that repo, the Git-Sync'd will check the repo for the commit message and use that as the commit message for the monitored file's commit to the sync'd repo. 

## Program Structure
### Service that runs all the time
There will need to be versions built for Windows and Linux. Mac support may come later depending on availability of a test platform. The service is the main part of the program that handles the monitoring and uploading/downloading of files. 
### [Command Line Interface](https://github.com/andrewmcdan/Git-Sync-d-CLI)
This will be built along side the Service. Initially, it will be the main UI. It will allow the user to add files and manage the repos associated with the service.
### GUI
Gonna take some inspiration from the Google Drive desktop app GUI. Taskbar icon and what-not. This GUI will be built later on. One thing the GUI needs is a quick an easy way to get the individual files that are sync'd and push/pull changes.

Interprocess communication will be handled by the Boost library's boost.interprocess implementation.

Encryption will handled by the Crypto++ Library for storing credentials on disk.

## Building

### Linux
```bash
mkdir build && cd build
cmake ..
make
```

### Windows
Use CMake with Visual Studio or the MSVC toolchain:
```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Installation

### Linux (systemd)
Copy the provided unit file and enable the service:
```bash
sudo cp scripts/linux/gitsyncd.service /etc/systemd/system/gitsyncd.service
sudo systemctl daemon-reload
sudo systemctl enable --now gitsyncd
```

### Windows
Register the service from an elevated command prompt:
```batch
scripts\windows\install_service.bat
```
This script uses `sc.exe` to register the service to run `Git-Sync-d --start`.

## 💖 Supporters
<!-- PATRONS:START -->
_(No public supporters yet)_
<!-- PATRONS:END -->
