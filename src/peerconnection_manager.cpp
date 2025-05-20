#include "peerconnection_manager.h"
#include "tunnel_loggin.h"

#include "peerconnection.h"

PeerconnectionManager::PeerconnectionManager(std::shared_ptr<RTCSignalingObserverInterface> signaling_observer)
    : _signaling_observer(std::move(signaling_observer))
{
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "PeerconnectionManager created";

    _stats_manager = rtc::make_ref_counted<StatsManager>();

    rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
}

PeerconnectionManager::~PeerconnectionManager()
{

}

void PeerconnectionManager::publish(std::string id, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    _local_id = id;

    rtc::scoped_refptr<SessionDescriptionObserver> session_desc_observer(new rtc::RefCountedObject<SessionDescriptionObserver>(id, *this));
    auto pc = rtc::make_ref_counted<Peerconnection>(session_desc_observer);
    pc->video_sink = sink;
    _peerconnections[id] = std::move(pc);
    _peerconnections[id]->publish();
}

void PeerconnectionManager::subscribe(const std::string& remote_id, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    rtc::scoped_refptr<SessionDescriptionObserver> session(new rtc::RefCountedObject<SessionDescriptionObserver>(std::string(remote_id), *this));
    auto pc = rtc::make_ref_counted<Peerconnection>(session);
    pc->video_sink = sink;
    _peerconnections[remote_id] = std::move(pc);
    _peerconnections[remote_id]->subscribe();
}

void PeerconnectionManager::handle_answer(const std::string& id, const std::string& sdp)
{
    auto it = _peerconnections.find(id);
    if(it != _peerconnections.end()) {
        it->second->set_remote_description(sdp);
    }
}

void PeerconnectionManager::on_local_description(const std::string& id)
{
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "On local desc " << id;
    auto pc = _peerconnections[id];
    std::string sdp;
    pc->get_local_description(sdp);

    if(id == _local_id) _signaling_observer->on_local_description(sdp);
    else _signaling_observer->on_local_description(id, sdp);
}

void PeerconnectionManager::on_remote_description(const std::string& id)
{
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "On remote desc " << id;

    if(id == _local_id) _signaling_observer->on_remote_description();
    else _signaling_observer->on_remote_description(id);
}

void PeerconnectionManager::stop(const std::string& id)
{
    /* auto it = _peerconnections.find(id);
    if(it != _peerconnections.end()) {
        it->second->stop();
        _peerconnections.erase(it);
    } */

    auto pc = _peerconnections[id];
    pc->stop();
    _peerconnections.erase(id);
}

void PeerconnectionManager::stop()
{
    for(auto& [id, pc] : _peerconnections) pc->stop();
    _peerconnections.clear();

    Peerconnection::clean();
}

void SessionDescriptionObserver::OnSetLocalDescriptionComplete(webrtc::RTCError error)
{
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "On set local desc complete";
    if(error.ok()) {
        _manager.on_local_description(_id);
    }
    else {
        TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Failed to set local description: " << error.message();
    }
}

void SessionDescriptionObserver::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
    if(error.ok()) {
        _manager.on_remote_description(_id);
    }
    else {
        TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Failed to set remote description: " << error.message();
    }
}

void StatsManager::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report)
{
    
}

// webrtc::webrtc_sequence_checker_internal::SequenceCheckerImpl::ExpectationToString() const

/* namespace webrtc::webrtc_sequence_checker_internal
{
    std::string SequenceCheckerImpl::ExpectationToString() const { return "wbla"; }

} */