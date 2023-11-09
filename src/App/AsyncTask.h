// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Jobs/Job.h"

namespace GRAPE {
    class AsyncTask {
    public:
        AsyncTask();
        ~AsyncTask();

        [[nodiscard]] bool running() const { return m_Running.load(); }
        [[nodiscard]] std::size_t queueSize() const { return m_Queue.size(); }
        [[nodiscard]] std::string message() const;

        template<typename Function>
        void pushTask(Function&& Func, const std::string& Message) {
            std::scoped_lock lck(m_Mutex);
            m_Queue.pushTask(Func);
            m_Messages.push_back(Message);
            m_Cv.notify_all();
        }

    private:
        std::thread m_Thread;
        std::condition_variable m_Cv;
        mutable std::mutex m_Mutex;
        std::atomic_bool m_Shutdown = false;

        std::string m_Message;
        std::deque<std::string> m_Messages;

        std::atomic_bool m_Running = false;

        MtQueue m_Queue;
    private:
        void asyncThread();
    };
}
