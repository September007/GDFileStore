#pragma once 
#include<functional>
#include<thread>
#include<vector>
#include<queue>
#include<future>
#include<assistant_utility.h>
using TaskType = std::function<void()>;
using std::vector;
using std::thread;
using std::queue;

namespace GD {

    class ThreadPool {
    public:
        explicit ThreadPool(size_t n) : shutdown_(false) {
            active(n);
        }

        ThreadPool(const ThreadPool&) = delete;
        ~ThreadPool() = default;

        void active(size_t n) {
            shutdown();
            unique_lock lg(mutex_);
            shutdown_ = false;
            threads_.clear();
            while (n) {
                threads_.emplace_back(worker(*this));
                n--;
            }
        }

        void enqueue(std::function<void()> fn) {
            std::unique_lock<std::mutex> lock(mutex_);
            jobs_.push_back(std::move(fn));
            cond_.notify_one();
        }

        void shutdown() {
            // Stop all worker threads...
            {
                std::unique_lock<std::mutex> lock(mutex_);
                shutdown_ = true;
            }

            cond_.notify_all();

            // Join...
            for (auto& t : threads_) {
                t.join();
            }
        }

        int workerRunning() {
            lock_guard lg(mutex_);
            return busyCount.load() + jobs_.size();
        }
    private:
        struct worker {
            explicit worker(ThreadPool& pool) : pool_(pool) {}

            void operator()() {
                for (;;) {
                    std::function<void()> fn;
                    {
                        std::unique_lock<std::mutex> lock(pool_.mutex_);

                        pool_.cond_.wait(
                            lock, [&] { return !pool_.jobs_.empty() || pool_.shutdown_; });

                        if (pool_.shutdown_ && pool_.jobs_.empty()) { break; }
                        pool_.busyCount++;
                        fn = pool_.jobs_.front();
                        pool_.jobs_.pop_front();
                    }
                    LOG_EXPECT_TRUE("ThreadPool",fn != nullptr);
                    fn();
                    pool_.busyCount--;
                }
            }

            ThreadPool& pool_;
        };
        friend struct worker;

        std::vector<std::thread> threads_;
        std::list<std::function<void()>> jobs_;

        bool shutdown_;

        std::condition_variable cond_;
        std::mutex mutex_;
        //record busy thread
        atomic_int busyCount = 0;
    };

}