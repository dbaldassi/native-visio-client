#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include <memory>
#include <string>
#include <thread>
#include <list>
#include <unordered_map>
#include <fstream>
#include <mutex>

#include <api/peer_connection_interface.h>
#include <api/scoped_refptr.h>
#include <api/media_stream_interface.h>

#include "session_description_obserser.h"

class Peerconnection : public webrtc::PeerConnectionObserver,
			  public webrtc::CreateSessionDescriptionObserver
{
  static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _pcf;
  static std::unique_ptr<rtc::Thread> _signaling_th;

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
  rtc::scoped_refptr<Peerconnection> _me;
  rtc::scoped_refptr<SessionDescriptionObserverInterface> _obs;

  void create_pc();

public:
  rtc::VideoSinkInterface<webrtc::VideoFrame> * video_sink = nullptr;

  struct RTCStats
  {
    int x;
    int bitrate;
    int link;
    int fps;    
    int frame_dropped = 0;
    int frame_decoded = 0;
    int frame_key_decoded = 0;
    int frame_rendered = 0;
    int frame_width = 0;
    int frame_height = 0;
    int delay = 0;
    int rtt = 0;
  };

  std::function<void(RTCStats)> onstats; 
  
  static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> get_pcf();
  static void clean();

  std::function<void(const std::string&)> onlocaldesc;
  std::list<RTCStats> stats;
  int link;

  struct PortRange
  {
    int min_port;
    int max_port;
  };
  
  std::optional<PortRange> port_range;
  
  Peerconnection(rtc::scoped_refptr<SessionDescriptionObserverInterface> obs);
  ~Peerconnection();
  
  void subscribe();
  void publish();

  void get_local_description(std::string &sdp);

  void stop();
  void set_remote_description(const std::string& sdp);

  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
  void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
  void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)  override;
  void OnRenegotiationNeeded() override;
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override;
  
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

public:

protected:
  mutable webrtc::webrtc_impl::RefCounter ref_count_{0};

};

#endif /* PEERCONNECTION_H */
