// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include <deque>
#include <mutex>
#include <thread>

namespace GRAPE {
    class Job {
    public:
        virtual ~Job() = default;

        virtual bool queue() = 0;
        virtual void run() = 0;
        virtual void stop() = 0;
        virtual void reset() = 0;

        virtual float progress() const { return 0.5f; }

        [[nodiscard]] bool ready() const { return m_Status.load() == Status::Ready; }
        [[nodiscard]] bool waiting() const { return m_Status.load() == Status::Waiting; }
        [[nodiscard]] bool running() const { return m_Status.load() == Status::Running; }
        [[nodiscard]] bool finished() const { return m_Status.load() == Status::Finished; }
        [[nodiscard]] bool stopped() const { return m_Status.load() == Status::Stopped; }

        void setFinished() { m_Status.store(Status::Finished); }
    protected:
        enum class Status {
            Ready = 0,
            Waiting,
            Running,
            Finished,
            Stopped,
        };
        std::atomic<Status> m_Status;
    };

    class MtQueue {
    public:
        MtQueue() = default;
        MtQueue(const MtQueue&) = delete;
        MtQueue(MtQueue&&) = delete;
        MtQueue& operator=(const MtQueue&) = delete;
        MtQueue& operator=(MtQueue&&) = delete;
        ~MtQueue() = default;

        [[nodiscard]] std::size_t size() const {
            std::scoped_lock lck(m_TasksMutex);
            return m_Tasks.size();
        }

        [[nodiscard]] bool empty() const {
            std::scoped_lock lck(m_TasksMutex);
            return m_Tasks.empty();
        }

        template<typename Function>
        void pushTask(Function&& Func) {
            std::scoped_lock lck(m_TasksMutex);
            m_Tasks.push_back(std::forward<Function>(Func));
        }

        std::function<void()> popTask() {
            std::scoped_lock lck(m_TasksMutex);

            if (m_Tasks.empty())
                return nullptr;

            auto task = std::move(m_Tasks.front());
            m_Tasks.pop_front();
            return task;
        }

        void clear() {
            std::scoped_lock lck(m_TasksMutex);
            m_Tasks.clear();
        }
    private:
        std::deque<std::function<void()>> m_Tasks{};
        mutable std::mutex m_TasksMutex;
    };

    class JobThread {
    public:
        explicit JobThread(MtQueue& Queue) : m_Queue(Queue) {}
        JobThread(const JobThread&) = delete;
        JobThread(JobThread&&) = delete;
        JobThread& operator=(const JobThread&) = delete;
        JobThread& operator=(JobThread&&) = delete;
        ~JobThread() {
            if (m_Thread.joinable())
                m_Thread.join();
        }

        void run() { m_Thread = std::thread([&] { job(); }); }
        void join() {
            if (m_Thread.joinable())
                m_Thread.join();
        }
    private:
        std::thread m_Thread;

        MtQueue& m_Queue;
    private:
        void job() const {
            while (auto task = m_Queue.popTask())
                task();
        }
    };
}
