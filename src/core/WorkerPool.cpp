#include "WorkerPool.h"

WorkerPool::WorkerPool(size_t threadCount)
{
    for (size_t i = 0; i < threadCount; i++)
    {
        workers.emplace_back([this]()
        {
            WorkerLoop();
        });
    }
}

WorkerPool::~WorkerPool()
{
    stop = true;
    condition.notify_all();

    for (auto& worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }
}

void WorkerPool::Enqueue(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks.push(task);
    }

    condition.notify_one();
}

void WorkerPool::WorkerLoop()
{
    while (!stop)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            condition.wait(lock, [this]
            {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}