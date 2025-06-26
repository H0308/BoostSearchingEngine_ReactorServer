#ifndef __rs_loop_thread_pool_h__
#define __rs_loop_thread_pool_h__

#include <boost_search/net/loop_thread.h>

namespace bs_loop_thread_pool
{
    class LoopThreadPool
    {
    public:
        using ptr = std::shared_ptr<LoopThreadPool>;

        LoopThreadPool(bs_event_loop_lock_queue::EventLoopLockQueue* loop)
            : base_loop_(loop), thread_num_(0), next_loop_(0)
        {
        }

        void createLoopThread()
        {
            // 当线程数量大于0时，创建从属线程
            if (thread_num_ > 0)
            {
                // 提前开辟空间便于创建每一个对象
                loop_threads_.resize(thread_num_);
                loops_.resize(thread_num_);
                // 创建从属线程
                for (int i = 0; i < thread_num_; i++)
                {
                    loop_threads_[i] = std::make_shared<bs_loop_thread::LoopThread>();
                    loops_[i] = loop_threads_[i]->getLoop();
                }
            }
        }

        void setThreadNum(int num)
        {
            thread_num_ = num;
        }

        bs_event_loop_lock_queue::EventLoopLockQueue* getNextLoop()
        {
            if (thread_num_ == 0)
                return base_loop_;
            return loops_[(next_loop_++) % thread_num_];
        }

    private:
        int thread_num_;                                                       // 线程个数
        int next_loop_;                                                        // 下一个从属事件循环监控
        bs_event_loop_lock_queue::EventLoopLockQueue* base_loop_;              // 主事件循环监控
        std::vector<bs_loop_thread::LoopThread::ptr> loop_threads_;            // 管理所有的线程事件监控
        std::vector<bs_event_loop_lock_queue::EventLoopLockQueue*> loops_; // 管理所有的事件循环监控
    };
}

#endif