#ifndef __rs_tcp_server_h__
#define __rs_tcp_server_h__

#include <unordered_map>
#include <boost_search/net/acceptor.h>
#include <boost_search/net/connection.h>
#include <boost_search/net/timing_wheel.h>
#include <boost_search/base/uuid_generator.h>
#include <boost_search/net/event_loop_lock_queue.h>
#include <boost_search/net/loop_thread_pool.h>

namespace bs_tcp_server
{
    class TcpServer
    {
    public:
        TcpServer(int port)
            : thread_num_(0), enable_timeout_release_(false), base_loop_(std::make_shared<bs_event_loop_lock_queue::EventLoopLockQueue>()), acceptor_(std::make_shared<bs_acceptor::Acceptor>(base_loop_.get(), port)), loop_pool_(std::make_shared<bs_loop_thread_pool::LoopThreadPool>(base_loop_.get()))
        {
            acceptor_->setAcceptCallback(std::bind(&TcpServer::handleAccept, this, std::placeholders::_1));
            acceptor_->enableConcerningAcceptFd();
        }

        void setThreadNum(int num)
        {
            thread_num_ = num;
            loop_pool_->setThreadNum(thread_num_);
        }

        void start()
        {
            loop_pool_->createLoopThread();
            base_loop_->startEventLoop();
        }

        void enableTimeoutRelease(uint32_t timeout)
        {
            timeout_ = timeout;
            enable_timeout_release_ = true;
        }

        void runTask(const bs_schedule_task::ScheduleTask::main_task_t &task, uint32_t timeout)
        {
            base_loop_->runTasks(std::bind(&TcpServer::runTaskInLoop, this, task, timeout));
        }

        void setConnectedCallback(const bs_connection::Connection::connectedCallback_t &cb)
        {
            con_cb_ = cb;
        }

        void setMessageCallback(const bs_connection::Connection::messageCallback_t &cb)
        {
            msg_cb_ = cb;
        }

        void setOuterCloseCallback(const bs_connection::Connection::closeCallback_t &cb)
        {
            outer_close_cb_ = cb;
        }

        void setAnyEventCallback(const bs_connection::Connection::anyEventCallback_t &cb)
        {
            any_cb_ = cb;
        }

    private:
        void handleAccept(int newfd)
        {
            // 创建客户端套接字结构
            const std::string id = rs_uuid_generator::UuidGenerator::generate_uuid();
            bs_connection::Connection::ptr client = std::make_shared<bs_connection::Connection>(loop_pool_->getNextLoop(), id, newfd);

            client->enableTimeoutRelease(10);

            client->setConnectedCallback(con_cb_);
            client->setMessageCallback(msg_cb_);
            client->setOuterCloseCallback(outer_close_cb_);
            client->setInnerCloseCallback(std::bind(&TcpServer::handleClose, this, std::placeholders::_1));
            client->establishAfterConnected();

            // 管理连接的客户端
            auto pos = conns_.try_emplace(id, client);
        }

        void handleClose(const bs_connection::Connection::ptr &con)
        {
            base_loop_->runTasks(std::bind(&TcpServer::handleCloseInLoop, this, con));
        }

        void handleCloseInLoop(const bs_connection::Connection::ptr &con)
        {
            std::string id = con->getId();
            auto pos = conns_.find(id);
            if (pos == conns_.end())
                return;
            conns_.erase(pos);
        }

        void runTaskInLoop(const bs_schedule_task::ScheduleTask::main_task_t &task, uint32_t timeout)
        {
            std::string id = rs_uuid_generator::UuidGenerator::generate_uuid();
            base_loop_->insertTask(id, timeout, task);
        }

    private:
        int thread_num_; 
        bool enable_timeout_release_;
        uint32_t timeout_;
        bs_event_loop_lock_queue::EventLoopLockQueue::ptr base_loop_;
        bs_acceptor::Acceptor::ptr acceptor_;
        bs_loop_thread_pool::LoopThreadPool::ptr loop_pool_;
        std::unordered_map<std::string, bs_connection::Connection::ptr> conns_;

        bs_connection::Connection::connectedCallback_t con_cb_;
        bs_connection::Connection::messageCallback_t msg_cb_;
        bs_connection::Connection::closeCallback_t outer_close_cb_;
        bs_connection::Connection::anyEventCallback_t any_cb_;
    };
}

#endif