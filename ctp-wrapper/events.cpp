#include "events.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

lueing::Events::Events()
{
}

lueing::Events::~Events()
{
}

void lueing::Events::WaitOnce(const std::string &event_name)
{
    boost::uuids::uuid u = boost::uuids::random_generator()();
    Wait(event_name, boost::uuids::to_string(u));
}

void lueing::Events::NotifyOnce(const std::string &event_name)
{
    Notify(event_name);
}

void lueing::Events::Wait(const std::string &event_name, const std::string &wait_id)
{
    std::unique_lock<std::mutex> lock(lock_);
    event_waiters_[event_name].emplace(wait_id);
    condition_lock_.wait(lock, [this, &event_name, &wait_id] {
        return !event_waiters_[event_name].contains(wait_id);
    });
}

void lueing::Events::Notify(const std::string &event_name)
{
    std::unique_lock<std::mutex> lock(lock_);
    for (const auto &waiter : event_waiters_[event_name])
    {
        event_waiters_[event_name].erase(waiter);
    }
    condition_lock_.notify_all();
}
