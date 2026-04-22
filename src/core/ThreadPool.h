#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    size_t size() const { return workers_.size(); }
    bool isActive() const { return !stop_; }
    
    void waitForCompletion();
    
private:
    std::vector<std::thread> workers_;
    
    std::queue<std::function<void()>> tasks_;
    
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable completion_condition_;
    std::atomic<bool> stop_;
    std::atomic<size_t> active_tasks_;
};

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        tasks_.emplace([this, task]() {
            active_tasks_++;
            (*task)();
            active_tasks_--;
            completion_condition_.notify_all();
        });
    }
    
    condition_.notify_one();
    return res;
}

namespace ParallelUtils {
    inline size_t calculateChunkSize(size_t total_items, size_t num_threads) {
        if (total_items == 0 || num_threads == 0) return 0;
        return std::max(size_t(1), total_items / (num_threads * 4));
    }
    
    inline size_t getOptimalThreadCount(size_t workload_size) {
        size_t hardware_threads = std::thread::hardware_concurrency();
        if (workload_size < hardware_threads) {
            return std::max(size_t(1), workload_size);
        }
        return hardware_threads;
    }
    
    class AtomicCounter {
    private:
        std::atomic<size_t> count_{0};
        size_t total_;
        
    public:
        AtomicCounter(size_t total) : total_(total) {}
        
        void increment() { count_++; }
        size_t get() const { return count_.load(); }
        double getProgress() const { 
            return total_ > 0 ? static_cast<double>(count_) / total_ : 0.0; 
        }
        bool isComplete() const { return count_ >= total_; }
    };
}

#endif