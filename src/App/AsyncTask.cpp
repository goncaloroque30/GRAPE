// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "AsyncTask.h"

namespace GRAPE {
    AsyncTask::AsyncTask() : m_Thread([&] { asyncThread(); }) {}
    AsyncTask::~AsyncTask() {
        std::unique_lock lck(m_Mutex);
        m_Queue.clear();
        m_Shutdown.store(true);
        m_Cv.notify_all();
        lck.unlock();

        if (m_Thread.joinable())
            m_Thread.join();
    }

    std::string AsyncTask::message() const {
        std::scoped_lock lck(m_Mutex);
        return m_Message;
    }

    void AsyncTask::asyncThread() {
        while (true)
        {
            std::unique_lock lck(m_Mutex);
            m_Cv.wait(lck, [this] { return m_Shutdown.load() || !m_Queue.empty(); });

            if (m_Shutdown.load())
                break;

            if (m_Queue.empty())
                continue;

            // Update Message
            m_Message = m_Messages.front();
            m_Messages.pop_front();

            auto task = m_Queue.popTask();
            lck.unlock();
            m_Running.store(true);
            task();
            if (m_Queue.empty())
                m_Running.store(false);
        }
    }
}
