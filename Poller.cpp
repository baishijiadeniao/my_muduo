#include "Poller.h"

Poller::Poller(EventLoop *loop):loop_(loop){
    
}

bool Poller::hasChannel(Channel* ch) const{
    auto it=channels_.find(ch->fd());
    return it != channels_.end() && ch==it->second;
}
