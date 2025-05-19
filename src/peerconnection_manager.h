#ifndef PEERCONNECTION_MANAGER_H
#define PEERCONNECTION_MANAGER_H

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
};

class StatsManager : public webrtc::RTCStatsCollectorCallback
{
    public:
        void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;
};

class PeerconnectionManager
{
    std::map<std::string, rtc::scoped_refptr<Peerconnection>, std::less<>> _peerconnections;
    std::string _local_id;
    std::shared_ptr<RTCSignalingObserverInterface> _signaling_observer;

    rtc::scoped_refptr<StatsManager> _stats_manager;

public:
    PeerconnectionManager(std::shared_ptr<RTCSignalingObserverInterface> signaling_observer);
    ~PeerconnectionManager();

    void stop(const std::string& id);
    void stop_all();

    void publish(std::string id, rtc::VideoSinkInterface<webrtc::VideoFrame> * sink);
    void subscribe(const std::string& remote_id, rtc::VideoSinkInterface<webrtc::VideoFrame> * sink);

    void handle_answer(const std::string& id, const std::string& sdp);

    void on_local_description(const std::string& id);
    void on_remote_description(const std::string& id);
};

#endif