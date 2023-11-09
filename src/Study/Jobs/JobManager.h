// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include <deque>
#include <mutex>
#include <semaphore>
#include <thread>

#include "Job.h"

namespace GRAPE {
    class JobManager {
    public:
        JobManager();
        JobManager(const JobManager&) = delete;
        JobManager(JobManager&&) = delete;
        JobManager& operator=(const JobManager&) = delete;
        JobManager& operator=(JobManager&&) = delete;
        ~JobManager();

        // Main Thread
        void queueJob(const std::shared_ptr<Job>& Jb);
        void waitForJobs();
        [[nodiscard]] bool isAnyRunning() const { return m_Running.load() != nullptr; }
        [[nodiscard]] bool isRunning(const std::shared_ptr<Job>& Jb) const { return m_Running.load() == Jb; }

        void resetJob(const std::shared_ptr<Job>& Jb);
        void shutdown();
    private:
        std::deque<std::shared_ptr<Job>> m_Jobs{};
        std::atomic<std::shared_ptr<Job>> m_Running = nullptr;

        mutable std::mutex m_Mutex;
        std::condition_variable m_JobAvailableCv;
        std::condition_variable m_JobDoneCv;
        std::atomic_bool m_Stop = false;

        std::binary_semaphore m_RunSemaphore;
        std::binary_semaphore m_WaitSemaphore;

        std::atomic_size_t m_TotalCount = 0;
        std::atomic_size_t m_QueuedAndRunning = 0;

        std::thread m_Thread;
    private:
        void threadLoop();
    };
}
