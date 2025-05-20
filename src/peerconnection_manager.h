#ifndef PEERCONNECTION_MANAGER_H
#define PEERCONNECTION_MANAGER_H

#include <thread>
#include <deque>

#include <api/stats/rtc_stats_collector_callback.h>
#include <api/media_stream_interface.h>

#include "session_description_obserser.h"

class PeerconnectionManager;
class Peerconnection;

class SessionDescriptionObserver : public  SessionDescriptionObserverInterface
{
    std::string _id;
    PeerconnectionManager& _manager;

public:
    SessionDescriptionObserver(std::string id, PeerconnectionManager& mgr) : _id(std::move(id)), _manager(mgr) {}

    void OnSetLocalDescriptionComplete(webrtc::RTCError error) override;
    void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;
};

class StatsCallback : public webrtc::RTCStatsCollectorCallback
{
public:
    struct RTCStats
    {
        uint64_t timestamp = 0;
        uint64_t bytes = 0;

        int bitrate = 0;
        int fps = 0;    
        int delay = 0;
        int rtt = 0;
    };

    StatsCallback(Peerconnection* pc, std::string id) : _pc(pc), _id(std::move(id)) {}
    ~StatsCallback() override = default;

    template<typename T>
    void on_stats_delivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report);

    std::string get_id() const { return _id; }
    RTCStats get_stats() const { return _stats; }

    void fetch();

private:
    RTCStats _stats;
    Peerconnection * _pc = nullptr;
    std::string _id;
};

class RTCSignalingObserverInterface
{
    public:
        virtual ~RTCSignalingObserverInterface() = default;
        // for publisher
        virtual void on_local_description(const std::string& sdp) = 0;
        virtual void on_remote_description() = 0;
        // for subscribers
        virtual void on_local_description(const std::string& id, const std::string& sdp) = 0;
        virtual void on_remote_description(const std::string& id) = 0;

        virtual void on_stats(const std::string& id, const std::vector<StatsCallback::RTCStats>& stats) = 0;
};

class InboundStatsCallback : public StatsCallback
{
public:
    InboundStatsCallback(Peerconnection* pc, std::string id) : StatsCallback(pc, std::move(id)) {}

    void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;
};

class OutboundStatsCallback : public StatsCallback
{
public:
    OutboundStatsCallback(Peerconnection* pc, std::string id) : StatsCallback(pc, std::move(id)) {}
    void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;
};

class PeerconnectionManager
{
    std::map<std::string, rtc::scoped_refptr<Peerconnection>, std::less<>> _peerconnections;
    std::string _local_id;
    std::shared_ptr<RTCSignalingObserverInterface> _signaling_observer;
    std::deque<rtc::scoped_refptr<StatsCallback>> _stats;
    std::jthread _stats_thread;

    static constexpr int STATS_INTERVAL = 500; // 500ms

public:
    PeerconnectionManager(std::shared_ptr<RTCSignalingObserverInterface> signaling_observer);
    ~PeerconnectionManager();

    void stop(const std::string& id);
    void stop();

    void publish(std::string id, rtc::VideoSinkInterface<webrtc::VideoFrame> * sink);
    void subscribe(const std::string& remote_id, rtc::VideoSinkInterface<webrtc::VideoFrame> * sink);

    void handle_answer(const std::string& id, const std::string& sdp);

    void on_local_description(const std::string& id);
    void on_remote_description(const std::string& id);
};

#endif