#pragma once
#include "TaskFunction.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

class Function;

class ThreadPool
{
    bool stop_ = false;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::vector<std::thread> vec_thread_;

    // 使用TaskFunction的原因：1.std::function无法包装std::packaged_task。2.TaskFunction实现了小对象优化，避免了多余的new
    std::deque<TaskFunction> deque_works_;

    explicit ThreadPool(size_t thread_num)
    {
        for (size_t i = 0; i < thread_num; i++) {
            vec_thread_.emplace_back([this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(mutex_);
                    condition_.wait(
                        lock, [this] { return !deque_works_.empty() || stop_; });
                    if (stop_ && deque_works_.empty())
                        return;
                    auto task_fun = std::move(deque_works_.front());
                    deque_works_.pop_front();
                    lock.unlock();
                    task_fun();
                }
            });
        }
    }

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (auto& thread : vec_thread_) {
            thread.join();
        }
    }

public:
    static ThreadPool& Init()
    {
        static ThreadPool pool{std::thread::hardware_concurrency() * 2 + 1};
        return pool;
    }

    // 有参数使用std::bind包装,没有返回本身。使用模板重载
    template<typename F, typename... Args>
    auto bindFun(F&& task_fun, Args&&... args) -> decltype(std::bind(std::forward<F>(task_fun), std::forward<Args>(args)...))
    {
        return std::bind(std::forward<F>(task_fun), std::forward<Args>(args)...);
    }

    template<typename F>
    F&& bindFun(F&& task_fun) noexcept
    {
        return std::forward<F>(task_fun);
    }

    template<typename F, typename... Args>
    auto submit(F&& task_fun, Args&&... args) -> std::future<decltype(bindFun(std::forward<F>(task_fun), std::forward<Args>(args)...)())>
    {
        using ResultType = decltype(bindFun(std::forward<F>(task_fun), std::forward<Args>(args)...)());
        std::packaged_task<ResultType()> pack_task_fun{bindFun(std::forward<F>(task_fun), std::forward<Args>(args)...)};
        auto ret_future{pack_task_fun.get_future()};
        // 添加到 deque 中
        {
            std::lock_guard<std::mutex> lock(mutex_);
            deque_works_.emplace_back(std::move(pack_task_fun));
        }
        // 通知一个等待中的线程，有新的任务可以执行了
        condition_.notify_one();

        return ret_future;
    }
};