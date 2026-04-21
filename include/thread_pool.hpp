#pragma once
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
namespace thread_pool {
class ThreadPool {
 public:
  ~ThreadPool()
  {
    _quit = true;
    for (auto& thread : _threads) {
      _queue_cv.notify_all();
      thread.join();
    }
  }
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;
  explicit ThreadPool(size_t ThreadNum)
  {
    _threads.reserve(ThreadNum);
    for (size_t i = 0; i < ThreadNum; ++i) {
      _threads.emplace_back(&ThreadPool::run, this);
    }
  }

  void waitAll()
  {
    std::unique_lock<std::mutex> lock(_queue_mtx);

    _completed_task_ids_cv.wait(lock, [this]() -> bool {
      std::lock_guard<std::mutex> task_lock(_completed_task_id_mtx);
      return _queue.empty() && _last_task == _completed_tasks.size();
    });
  }
  template <typename Func, typename... Args>
  uint64_t addTask(const Func& task_func, Args&&... args)
  {
    uint64_t task_idx = _last_task++;

    std::lock_guard<std::mutex> q_lock(_queue_mtx);

    _queue.emplace(std::async(std::launch::deferred, task_func, std::forward<Args>(args)...),
                   task_idx);

    _queue_cv.notify_one();
    return task_idx;
  }

 private:
  void run()
  {
    while (!_quit) {
      std::unique_lock<std::mutex> lock(_queue_mtx);

      _queue_cv.wait(lock, [this]() { return !_queue.empty() || _quit; });

      if (!_queue.empty()) {
        auto element = std::move(_queue.front());

        _queue.pop();
        lock.unlock();

        element.first.get();
        std::lock_guard<std::mutex> lock_g(_completed_task_id_mtx);
        _completed_tasks.insert(element.second);
        _completed_task_ids_cv.notify_all();
      }
    }
  }

  std::queue<std::pair<std::future<void>, uint64_t>> _queue;
  std::vector<std::thread> _threads;
  std::unordered_set<uint64_t> _completed_tasks;
  std::condition_variable _queue_cv;
  std::condition_variable _completed_task_ids_cv;
  std::mutex _queue_mtx;
  std::mutex _completed_task_id_mtx;
  std::atomic<uint64_t> _last_task{0};
  std::atomic<bool> _quit{false};
};
}  // namespace thread_pool
