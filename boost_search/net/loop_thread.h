#ifndef __rs_loop_thread_h__
#define __rs_loop_thread_h__

#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost_search/net/event_loop_lock_queue.h>

namespace bs_loop_thread
{
    class LoopThread
    {
    public:
        using ptr = std::shared_ptr<LoopThread>;

        LoopThread()
            : thread_(std::thread(std::bind(&LoopThread::threadEntry, this))), loop_(nullptr)
        {

        }

        bs_event_loop_lock_queue::EventLoopLockQueue* getLoop()
        {
            bs_event_loop_lock_queue::EventLoopLockQueue::ptr loop;
            {
                std::unique_lock<std::mutex> lock(loop_mtx_);
                // 如果loop_为空，则在条件变量上等待直到loop_不为空
                loop_con_.wait(lock, [&](){
                    return static_cast<bool>(loop_);
                });
                loop = loop_;
            }

            return loop.get();
        }
    private:
        void threadEntry()
        {
            // 实例化EventLoop对象，再启动事件监控
            bs_event_loop_lock_queue::EventLoopLockQueue::ptr loop = std::make_shared<bs_event_loop_lock_queue::EventLoopLockQueue>();
            {
                std::unique_lock<std::mutex> lock(loop_mtx_);
                loop_ = loop;
                loop_con_.notify_all();
            }

            loop_->startEventLoop();
        }

    private:
        std::mutex loop_mtx_;
        std::condition_variable loop_con_;
        std::thread thread_;
        bs_event_loop_lock_queue::EventLoopLockQueue::ptr loop_;
    };
}

#endif