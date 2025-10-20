#include "ThreadPool.h"
 

ThreadPool::ThreadPool(size_t numThreads) :
    maxNumThreads_(numThreads)
{
    start(numThreads);
}

ThreadPool::~ThreadPool() 
{ 
    stop(); 
}

size_t ThreadPool::getNumThreads() const
{
    return maxNumThreads_;
}

void ThreadPool::start(size_t numThreads)
{
    for (auto i = 0; i < numThreads; ++i)
    {
        threads_.emplace_back([=] {
            while (true)
            {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(eventMutex_);
                    eventVar_.wait(lock, [=]() { return stopping_ || !tasks_.empty(); });
                    if (stopping_ && tasks_.empty())
                        break;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
            });
    }
}

void ThreadPool::stop() noexcept
{
    {
        std::unique_lock<std::mutex> lock(eventMutex_);
        stopping_ = true;
    }
    eventVar_.notify_all();
    for (auto& thread : threads_)
        thread.join();
}