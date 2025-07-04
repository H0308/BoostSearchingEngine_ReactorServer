#ifndef __rs_poller_h__
#define __rs_poller_h__

#include <sys/epoll.h>
#include <array>
#include <unordered_map>
#include <vector>
#include <boost_search/base/log.h>
#include <boost_search/base/error.h>
#include <boost_search/net/channel.h>

namespace bs_poller
{
    using namespace bs_log_system;

    const int max_ready_events = 1024;

    // 对epoll操作的封装以及上层使用简化
    class Poller
    {
    public:
        using ptr = std::shared_ptr<Poller>;

        Poller()
        {
            epfd_ = epoll_create(256);
            if(epfd_ < 0)
            {
                LOG(Level::Error, "创建Epoll模型失败");
                exit(static_cast<int>(rs_error::ErrorNum::Epoll_create_fail));
            }
        }

        // 添加/更新指定描述符的事件监控
        void updateEvent(bs_channel::Channel::ptr channel)
        {
            // 存在就更新，不存在就添加
            int fd = channel->getFd();
            auto pos = channels_.try_emplace(fd, channel);
            if(pos.second)
                update(EPOLL_CTL_ADD, channel);
            else
                update(EPOLL_CTL_MOD, channel);
        }

        // 移除指定描述符的事件监控
        void removeEvent(bs_channel::Channel::ptr channel)
        {
            auto it = channels_.find(channel->getFd());
            if(it == channels_.end())
                return;
            update(EPOLL_CTL_DEL, channel);
            channels_.erase(channel->getFd());
        }

        // 开启监控并获取就绪数组
        int startEpoll(std::vector<bs_channel::Channel::ptr> &channels)
        {
            // 阻塞等待
            int nfds = epoll_wait(epfd_, epoll_events_.data(), max_ready_events, -1);
            if(nfds < 0)
            {
                // 被中断打断，属于可接受范围
                if(errno == EINTR)
                    return 0;
                LOG(Level::Error, "事件等待失败：{}", strerror(errno));
                exit(static_cast<int>(rs_error::ErrorNum::Epoll_wait_fail));
            }

            // 等待成功将就绪的事件监控结构返回
            for(int i = 0; i < nfds; i++)
            {
                // 判断指定文件描述符是否存在
                auto it = channels_.find(epoll_events_[i].data.fd);
                assert(it != channels_.end());
                // 存在再设置对应的就绪事件
                auto channel = it->second;
                channel->setReadyEvents(epoll_events_[i].events);
                channels.emplace_back(channel);
            }

            return nfds;
        }

    private:
        // 直接进行epoll_ctl的操作封装
        void update(int op, bs_channel::Channel::ptr channel)
        {
            int fd = channel->getFd();
            struct epoll_event ev;
            ev.data.fd = fd;
            ev.events = channel->getEvents();
            int ret = epoll_ctl(epfd_, op, fd, &ev);
            if(ret < 0)
            {
                LOG(Level::Error, "添加文件描述符监控失败");
                // exit(static_cast<int>(rs_error::ErrorNum::Epoll_ctl_fail));
            }
        }

    private:
        int epfd_;                                                     // epoll文件描述符
        std::array<struct epoll_event, max_ready_events> epoll_events_; // 就绪事件数组
        std::unordered_map<int, bs_channel::Channel::ptr> channels_;    // 管理的事件监控结构
    };
}

#endif