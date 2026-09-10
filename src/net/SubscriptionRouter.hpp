#pragma once
#include "Protocol.hpp"
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace Net {

    enum class Scope : uint8_t {
        PerProject,
        PerSession,
    };

    struct ResourceID {
        Scope     scope;
        ProjectID projectID;
        SessionID sessionID;
        uint32_t  typeTag;
        uint64_t  instanceID;

        bool operator==(const ResourceID& o) const {
            return scope == o.scope
                && projectID == o.projectID
                && sessionID == o.sessionID
                && typeTag == o.typeTag
                && instanceID == o.instanceID;
        }
    };

    struct ResourceHash {
        size_t operator()(const ResourceID& r) const noexcept {
            size_t h = 1469598103934665603ull;                 // FNV-1a 64-bit
            auto mix = [&h](uint64_t v) {
                h ^= v;
                h *= 1099511628211ull;
                };
            mix((uint64_t)r.scope);
            mix(r.projectID);
            mix(r.sessionID);
            mix((uint64_t)r.typeTag);
            mix(r.instanceID);
            return h;
        }
    };

    class SubscriptionRouter {
    public:
        using Deliver = std::function<void(SessionID, const void*, size_t)>;

        void SetDeliver(Deliver d) { deliver = std::move(d); }

        void Subscribe(SessionID s, const ResourceID& r) {
            subs[r].insert(s);
            sessionSubs[s].insert(r);
        }

        void UnsubscribeAll(SessionID s) {
            auto it = sessionSubs.find(s);
            if (it == sessionSubs.end()) return;
            for (const auto& r : it->second) {
                auto sub = subs.find(r);
                if (sub != subs.end()) {
                    sub->second.erase(s);
                    if (sub->second.empty()) subs.erase(sub);
                }
            }
            sessionSubs.erase(it);
        }

        void Publish(const ResourceID& r, const void* payload, size_t size) {
            auto it = subs.find(r);
            if (it == subs.end()) return;
            for (SessionID s : it->second) deliver(s, payload, size);
        }

        void PublishToProject(ProjectID p, uint32_t typeTag,
            const void* payload, size_t size) {
            ResourceID r{ Scope::PerProject, p, 0, typeTag, 0 };
            Publish(r, payload, size);
        }

    private:
        // unordered_set, not set: we have a hash, not a comparator
        std::unordered_map<ResourceID, std::unordered_set<SessionID>, ResourceHash> subs;
        std::unordered_map<SessionID, std::unordered_set<ResourceID, ResourceHash>> sessionSubs;
        Deliver deliver;
    };

} // namespace Net