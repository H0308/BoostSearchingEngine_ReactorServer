#ifndef __rs_channel_h__
#define __rs_channel_h__

#include <cstdint>
#include <functional>
#include <sys/epoll.h>
#include <memory>

namespace bs_event_loop_lock_queue
{
    class EventLoopLockQueue;
}

namespace bs_channel
{
    // 事件处理回调（参数后续设置）
    using event_callback_t = std::function<void()>;

    class Channel : public std::enable_shared_from_this<Channel>
    {
    public:
        using ptr = std::shared_ptr<Channel>;

        Channel(bs_event_loop_lock_queue::EventLoopLockQueue* loop, int fd)
            : fd_(fd), events_(0), revents_(0), loop_(loop)
        {
        }

        // 是否已经关心可读
        bool checkIsConcerningReadFd()
        {
            return static_cast<bool>(events_ & EPOLLIN);
        }

        // 是否已经关心可写
        bool checkIsConcerningWriteFd()
        {
            return static_cast<bool>(events_ & EPOLLOUT);
        }

        // 启用读事件关心
        void enableConcerningReadFd()
        {
            events_ |= EPOLLIN;
            update();
        }

        // 启用写事件关心
        void enableConcerningWriteFd()
        {
            events_ |= EPOLLOUT;
            update();
        }

        // 关闭读事件关心
        void disableConcerningReadFd()
        {
            events_ &= ~EPOLLIN;
            update();
        }

        // 关闭写事件
        void disableConcerningWriteFd()
        {
            events_ &= ~EPOLLOUT;
            update();
        }

        // 关闭所有事件关心
        void disableConcerningAll()
        {
            events_ = 0;
            update();
        }

        // 移动指定文件描述符关心
        void removeFd()
        {
            // 待完善具体操作
            remove();
        }

        void remove();
        void update();

        // 根据具体时间调用对应的回调函数
        void handleEvent()
        {
            // 先读后写，错误优先，关闭最后
            if ((revents_ & EPOLLIN) || (revents_ & EPOLLRDHUP) || (revents_ & EPOLLPRI))
            {
                if (read_cb_)
                    read_cb_();
            }
            if (revents_ & EPOLLOUT)
            {
                if (write_cb_)
                    write_cb_();
            }
            else if (revents_ & EPOLLERR)
            {
                if (error_cb_)
                    error_cb_();
            }
            else if (revents_ & EPOLLHUP)
            {
                if (close_cb_)
                    close_cb_();
            }
            // 任意事件回调
            if (any_cb_)
                any_cb_();
        }

        // 设置读事件回调
        void setReadCallback(const event_callback_t &cb)
        {
            read_cb_ = cb;
        }

        // 设置写事件回调
        void setWriteCallback(const event_callback_t &cb)
        {
            write_cb_ = cb;
        }

        // 设置错误事件回调
        void setErrorCallback(const event_callback_t &cb)
        {
            error_cb_ = cb;
        }

        // 设置连接断开事件回调
        void setCloseCallback(const event_callback_t &cb)
        {
            close_cb_ = cb;
        }

        // 设置任意时间回调
        void setAnyCallback(const event_callback_t &cb)
        {
            any_cb_ = cb;
        }

        // 设置就绪事件
        void setReadyEvents(uint32_t revents)
        {
            revents_ = revents;
        }

        // 获取当前文件描述符
        int getFd()
        {
            return fd_;
        }

        // 获取已经管理的事件
        uint32_t getEvents()
        {
            return events_;
        }

        ~Channel()
        {
            // Channel不负责EventLoop的生命周期，只是使用EventLoop
        }

    private:
        int fd_;           // 指定的文件描述符
        uint32_t events_;  // 关心的事件
        uint32_t revents_; // 已经就绪的事件

        event_callback_t read_cb_;  // 读事件回调
        event_callback_t write_cb_; // 写事件回调
        event_callback_t error_cb_; // 错误事件回调
        event_callback_t close_cb_; // 连接断开事件回调
        event_callback_t any_cb_;   // 任意事件回调

        bs_event_loop_lock_queue::EventLoopLockQueue* loop_;
    };
}

#endif