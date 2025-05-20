#include <ranges>

#include "peerconnection_manager.h"
#include "tunnel_loggin.h"

#include "peerconnection.h"

#include "api/stats/rtcstats_objects.h"

PeerconnectionManager::PeerconnectionManager(std::shared_ptr<RTCSignalingObserverInterface> signaling_observer)
    : _signaling_observer(std::move(signaling_observer))
{
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "PeerconnectionManager created";

    _stats_thread = std::jthread([this](std::stop_token st) {
        while(!st.stop_requested()) {
            for(auto& cb : _stats) cb->fetch();
            std::this_thread::sleep_for(std::chrono::milliseconds(STATS_INTERVAL / 5));
            auto stats = _stats | std::views::transform([](const auto& cb) { return cb->get_stats(); })
            | std::ranges::to<std::vector<StatsCallback::RTCStats>>();
            _signaling_observer->on_stats(_local_id, stats);
            std::this_thread::sleep_for(std::chrono::milliseconds(4 * STATS_INTERVAL / 5));
        }
    });

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

    auto stats = rtc::make_ref_counted<OutboundStatsCallback>( _peerconnections[id].get(), id);
    _stats.push_front(std::move(stats));
}

void PeerconnectionManager::subscribe(const std::string& remote_id, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    rtc::scoped_refptr<SessionDescriptionObserver> session(new rtc::RefCountedObject<SessionDescriptionObserver>(std::string(remote_id), *this));
    auto pc = rtc::make_ref_counted<Peerconnection>(session);
    pc->video_sink = sink;
    _peerconnections[remote_id] = std::move(pc);
    _peerconnections[remote_id]->subscribe();

    auto stats = rtc::make_ref_counted<InboundStatsCallback>(_peerconnections[remote_id].get(), remote_id);
    _stats.push_back(std::move(stats));
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
    auto it = std::find_if(_stats.begin(), _stats.end(), [&id](const auto& cb) {
        return cb->get_id() == id;
    });

    if(it != _stats.end()) _stats.erase(it);

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

template<typename T>
void StatsCallback::on_stats_delivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report)
{
    auto stats = report->GetStatsOfType<T>();

    uint64_t bytes = 0;
    uint64_t fps = 0;

    auto ts = report->timestamp();
    auto ts_ms = ts.ms();
    auto delta = ts_ms - _stats.timestamp;

    for(const auto& s : stats) {
        if(*s->kind == webrtc::RTCMediaStreamTrackKind::kVideo) {
            if constexpr (std::is_same_v<T, webrtc::RTCInboundRTPStreamStats>) {
                bytes += *s->bytes_received;
            } else if constexpr (std::is_same_v<T, webrtc::RTCOutboundRTPStreamStats>) {
                bytes += *s->bytes_sent;
            }

            fps += s->frames_per_second.ValueOrDefault(0.);
        }
    }

    auto delta_bytes = bytes - _stats.bytes;
    _stats.bytes = bytes;
    _stats.timestamp = ts_ms;
    _stats.bitrate = static_cast<int>(8. * delta_bytes / delta);
    _stats.fps = fps / stats.size();

    auto remote_stats = report->GetStatsOfType<std::conditional_t<std::is_same_v<T, webrtc::RTCInboundRTPStreamStats>,
    webrtc::RTCRemoteOutboundRtpStreamStats,
    webrtc::RTCRemoteInboundRtpStreamStats>>();

    _stats.rtt = 0;

    for(const auto& s : remote_stats) {
        if(*s->kind == webrtc::RTCMediaStreamTrackKind::kVideo) {
            auto rtt = s->round_trip_time.ValueOrDefault(0.) * 1000;
            if(rtt > _stats.rtt) _stats.rtt = static_cast<int>(rtt);
        }
    }
}

void StatsCallback::fetch()
{
    _pc->get_stats(this);
}

void InboundStatsCallback::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report)
{
    on_stats_delivered<webrtc::RTCInboundRTPStreamStats>(report);
}

void OutboundStatsCallback::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report)
{
    on_stats_delivered<webrtc::RTCOutboundRTPStreamStats>(report);
}