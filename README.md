# Git-Sync'd
Git client for syncing individual files to Git repos. 

## The Idea
So, if you have several files that you want to sync to a repo, but don't want to init the whole folder that the file exists in, or maybe you want to have a repo that has all the .gitignore files, this tool will sync individual files to a repo. 

Let's say you have several repos and each one has a .gitignore file that you want to collect into one repo. This tool can sync all these .gitignore files into that repo and whenever it does so, it uses the git commit message from the git repo the .gitignore exists in.
