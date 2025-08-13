#include "mainLogic.h"
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace MainLogic_H
{
    GIT_SYNC_D_MESSAGE::Error globalErrors(1000);
    MainLogic mainLogic;
    // Flag indicating that the list of watched files/directories has been
    // modified and the internal caches should be refreshed.
    static std::atomic<bool> restartWatchers{false};

    bool loop()
    {
        static std::unordered_map<int, std::filesystem::file_time_type> lastWrite;
        static std::unordered_map<int, std::string> lastCommit;

        if (restartWatchers.load())
        {
            // Clearing these maps effectively resets our file watching state.
            lastWrite.clear();
            lastCommit.clear();
            restartWatchers = false;
        }

        // this is the main service loop.
        // it will be called by the service/daemon
        // and will run until the service/daemon is stopped.
        while (mainLogic.isRunning())
        {
            // Fetch the list of paths we need to watch from the database.
            auto entries = DB::listSyncEntries();
            for (const auto &entry : entries)
            {
                std::filesystem::path src(entry.filePath);
                std::filesystem::path dstRepo(entry.repoPath);
                bool repoMode = entry.options.find("repo") != std::string::npos;
                std::string srcRepo = GitUtils::findRepoRoot(src);
                try
                {
                    if (repoMode && !srcRepo.empty())
                    {
                        std::string msg =
                            GitUtils::getLastCommitMessage(srcRepo, src.string());
                        if (!msg.empty() && lastCommit[entry.id] != msg)
                        {
                            lastCommit[entry.id] = msg;
                            std::filesystem::copy_file(
                                src, dstRepo / src.filename(),
                                std::filesystem::copy_options::overwrite_existing);
                            std::string target = (dstRepo / src.filename()).string();
                            if (GitUtils::stageFile(dstRepo.string(), target) != 0) {
                                GIT_SYNC_D_MESSAGE::Error::error(
                                    "Failed to stage file",
                                    GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_ERROR);
                            }
                            if (GitUtils::commit(dstRepo.string(), msg) != 0) {
                                GIT_SYNC_D_MESSAGE::Error::error(
                                    "Failed to commit changes",
                                    GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_ERROR);
                            }
                            if (GitUtils::push(dstRepo.string(), "origin") != 0) {
                                GIT_SYNC_D_MESSAGE::Error::error(
                                    "Failed to push changes",
                                    GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_ERROR);
                            }
                        }
                    }
                    else
                    {
                        auto current = std::filesystem::last_write_time(src);
                        if (lastWrite[entry.id] != current)
                        {
                            lastWrite[entry.id] = current;
                            std::string msg;
                            if (!srcRepo.empty())
                            {
                                msg = GitUtils::getLastCommitMessage(srcRepo,
                                                                     src.string());
                            }
                            if (msg.empty())
                            {
                                msg = "Auto-sync";
                            }
                            std::filesystem::copy_file(
                                src, dstRepo / src.filename(),
                                std::filesystem::copy_options::overwrite_existing);
                            std::string target = (dstRepo / src.filename()).string();
                            if (GitUtils::stageFile(dstRepo.string(), target) != 0) {
                                GIT_SYNC_D_MESSAGE::Error::error(
                                    "Failed to stage file",
                                    GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_ERROR);
                            }
                            if (GitUtils::commit(dstRepo.string(), msg) != 0) {
                                GIT_SYNC_D_MESSAGE::Error::error(
                                    "Failed to commit changes",
                                    GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_ERROR);
                            }
                            if (GitUtils::push(dstRepo.string(), "origin") != 0) {
                                GIT_SYNC_D_MESSAGE::Error::error(
                                    "Failed to push changes",
                                    GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_ERROR);
                            }
                        }
                    }
                }
                catch (const std::filesystem::filesystem_error &e)
                {
                    GIT_SYNC_D_MESSAGE::Error::error(
                        e.what(), GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_ERROR);
                }
            }

            // check to see if we have we received a shutdown command
            if (!mainLogic.ipc->running)
            {
            }
            if (IPC::shutdown_trigger)
            {
                mainLogic.stop();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        if (mainLogic.stopped)
        {
            // stop all the things

            // once everything is stopped...
            mainLogic.stopConfirmed();
            return false; // this will break the service / daemon out of its loop
        }
        return true;
    }

    void stop()
    {
        mainLogic.stop();
    }

    bool IsServiceStopped()
    {
        return true;
    }

    void MainLogic::setRunning(bool _running)
    {
        mainLogic.running = _running;
    }

    MainLogic::MainLogic()
    {
        mainLogic.setRunning(true);
        ipc = new IPC();
    }

    MainLogic::~MainLogic()
    {
        mainLogic.setRunning(false);
        ipc->~IPC();
    }

    bool MainLogic::isRunning()
    {
        return mainLogic.running;
    }

    void MainLogic::stop()
    {
        mainLogic.running = false;
    }

    void setLogEvent(std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> _logEvent)
    {
        // sysLogEvent = _logEvent;
        GIT_SYNC_D_MESSAGE::Error::setSysLog(_logEvent);
    }

    void MainLogic::stopConfirmed()
    {
        mainLogic.stopped = true;
        // sysLogEvent("Git Sync'd has stopped", GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_INFO);
        GIT_SYNC_D_MESSAGE::Error::error("Git Sync'd has stopped", GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_INFO);
    }

    // ---- Synchronisation management helpers ---------------------------------

    bool addFile(const std::string &filePath,
                 const std::string &repoPath,
                 const std::string &options)
    {
        bool ok = DB::addSyncEntry(filePath, repoPath, options);
        if (ok)
        {
            restartWatchers = true;
        }
        return ok;
    }

    bool addDirectory(const std::string &dirPath,
                      const std::string &repoPath,
                      const std::string &options)
    {
        bool ok = DB::addSyncEntry(dirPath, repoPath, options);
        if (ok)
        {
            restartWatchers = true;
        }
        return ok;
    }

    bool removeSync(const std::string &path)
    {
        auto entries = DB::listSyncEntries();
        for (const auto &e : entries)
        {
            if (e.filePath == path)
            {
                bool ok = DB::removeSyncEntry(e.id);
                if (ok)
                    restartWatchers = true;
                return ok;
            }
        }
        return false;
    }

} // namespace MainLogic_H
