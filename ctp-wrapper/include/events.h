#ifndef LUEING_DATA_PROVIDER_CTP_EVENTS_H
#define LUEING_DATA_PROVIDER_CTP_EVENTS_H

#include <string>
#include <absl/container/node_hash_map.h>
#include <absl/container/node_hash_set.h>
#include <mutex>
#include <condition_variable>

namespace lueing {
    #define EVENT_LOGIN "event_login"

    class Events
    {
    private:
        absl::node_hash_map<std::string, absl::node_hash_set<std::string>> event_waiters_;
        std::mutex lock_;
        std::condition_variable condition_lock_;
    public:
        Events(/* args */);
        ~Events();

    public:
        void WaitOnce(const std::string& event_name);
        void NotifyOnce(const std::string& event_name);

        void Wait(const std::string& event_name, const std::string& wait_id);
        void Notify(const std::string& event_name);
    };    
} // namespace lueing

#endif // LUEING_DATA_PROVIDER_CTP_EVENTS_H