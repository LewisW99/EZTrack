#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class WorkerPool
{
public:

    WorkerPool(size_t threadCount);
    ~WorkerPool();

    void Enqueue(std::function<void()> task);

private:

    void WorkerLoop();

    std::vector<std::thread> workers;

    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;

    std::atomic<bool> stop{false};
};