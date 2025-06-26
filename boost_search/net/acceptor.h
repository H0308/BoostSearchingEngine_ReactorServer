#ifndef __rs_acceptor_h__
#define __rs_acceptor_h__

#include <boost_search/net/socket.h>
#include <boost_search/net/event_loop_lock_queue.h>
#include <boost_search/net/channel.h>

namespace bs_acceptor
{
    /**
     * 当前类不处理Connection的创建，当前类只是接收连接，获取到对应的连接描述符
     * 具体如何处理连接描述符交给上层（服务器模块）处理
     */
    class Acceptor
    {
    public:
        using ptr = std::shared_ptr<Acceptor>;
        // 连接文件描述符处理回调
        using acceptCallback_t = std::function<void(int)>;

        Acceptor(bs_event_loop_lock_queue::EventLoopLockQueue* loop, int port)
            : loop_(loop), channel_(std::make_shared<bs_channel::Channel>(loop_, getAcceptFd(port)))
        {
            channel_->setReadCallback(std::bind(&Acceptor::handleAccept, this));
        }

        void setAcceptCallback(const acceptCallback_t &cb)
        {
            ac_cb_ = cb;
        }

        void enableConcerningAcceptFd()
        {
            channel_->enableConcerningReadFd();
        }

    private:
        // 处理有新连接的回调函数
        void handleAccept()
        {
            // 获取新连接并交给上层处理
            int newfd = socket_->accept();
            if (ac_cb_)
                ac_cb_(newfd);
        }

        // 获取监听套接字文件描述符
        int getAcceptFd(int port)
        {
            socket_ = std::make_shared<bs_socket::Socket>();
            bool ret = socket_->createServer(port, true);
            assert(ret);
            return socket_->getSockFd();
        }

    private:
        bs_socket::Socket::ptr socket_;                          // 套接字操作
        bs_event_loop_lock_queue::EventLoopLockQueue* loop_; // 监听套接字描述符事件监控
        bs_channel::Channel::ptr channel_;                       // 监听套接字描述符事件管理

        acceptCallback_t ac_cb_;
    };
}

#endif