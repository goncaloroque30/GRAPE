// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "JobManager.h"

namespace GRAPE {
    JobManager::JobManager() : m_RunSemaphore(1), m_WaitSemaphore(1), m_Thread([&] { threadLoop(); }) {}

    JobManager::~JobManager() {}

    void JobManager::queueJob(const std::shared_ptr<Job>& Jb) {
        if (!Jb->queue())
            return;
        std::scoped_lock lck(m_Mutex);
        ++m_QueuedAndRunning;
        ++m_TotalCount;
        m_Jobs.push_back(Jb);
        m_JobAvailableCv.notify_all();
    }

    void JobManager::waitForJobs() {
        if (m_QueuedAndRunning == 0)
            return;

        std::unique_lock lck(m_Mutex);
        m_JobDoneCv.wait(lck, [this] { return m_QueuedAndRunning == 0; });
    }

    void JobManager::resetJob(const std::shared_ptr<Job>& Jb) {
        if (!Jb)
            return;

        const bool running = m_Running.load() == Jb;
        m_WaitSemaphore.acquire();
        Jb->stop();
        if (running)
            m_RunSemaphore.acquire();
        Jb->reset();
        if (running)
            m_RunSemaphore.release();
        m_WaitSemaphore.release();
    }

    void JobManager::shutdown() {
        std::unique_lock lck(m_Mutex);
        m_Jobs.clear();
        m_Stop.store(true);
        m_JobAvailableCv.notify_all();
        lck.unlock();

        resetJob(m_Running);
        m_Thread.join();
    }

    void JobManager::threadLoop() {
        while (true)
        {
            std::unique_lock lck(m_Mutex);
            m_JobAvailableCv.wait(lck, [this] { return m_Stop.load() || !m_Jobs.empty(); });

            if (m_Stop.load())
                break;

            if (!m_Jobs.empty())
            {
                m_Running.store(m_Jobs.front());
                m_Jobs.pop_front();
                lck.unlock();
                m_RunSemaphore.acquire();
                const auto runJob = m_Running.load();
                if (runJob->waiting())
                    runJob->run();
                m_Running.store(nullptr);
                --m_QueuedAndRunning;
                m_RunSemaphore.release();
                m_WaitSemaphore.acquire();
                m_WaitSemaphore.release();
                m_JobDoneCv.notify_all();
            }
        }
    }
}
