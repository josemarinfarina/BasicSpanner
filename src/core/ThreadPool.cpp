#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t threads)
    : stop_(false), active_tasks_(0) {
    
    if (threads == 0) {
        threads = 1;
    }
    
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);
                    this->condition_.wait(lock,
                        [this]{ return this->stop_ || !this->tasks_.empty(); });
                    
                    if (this->stop_ && this->tasks_.empty()) {
                        return;
                    }
                    
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "ThreadPool: Exception in worker thread: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "ThreadPool: Unknown exception in worker thread" << std::endl;
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    
    condition_.notify_all();
    
    for (std::thread &worker: workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::waitForCompletion() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    completion_condition_.wait(lock, [this] {
        return tasks_.empty() && active_tasks_ == 0;
    });
} 