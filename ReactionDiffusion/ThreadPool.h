#pragma once
#include <functional>
#include <future>
#include <queue>


class ThreadPool
{
public:
    using Task = std::function<void()>;

    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    size_t getNumThreads() const;

    template<class T>
    auto enqueue(T task) -> std::future<decltype(task())>
    {
        auto wrapper = std::make_shared<std::packaged_task<decltype(task())()>>(std::move(task));
        {
            std::unique_lock<std::mutex> lock(eventMutex_);
            tasks_.emplace([=] {
                (*wrapper)();
                });
        }

        eventVar_.notify_one();
        return wrapper->get_future();
    }


private:
    void start(size_t numThreads);
    void stop() noexcept;

    std::vector<std::thread> threads_;
    std::queue<Task> tasks_;
    std::condition_variable eventVar_;
    std::mutex eventMutex_;
    bool stopping_ = false;
    size_t maxNumThreads_ = 1;
};

