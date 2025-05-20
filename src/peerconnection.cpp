#include "peerconnection.h"

#include <stdexcept>
#include <iostream>
#include <chrono>
#include <algorithm>

#include <api/create_peerconnection_factory.h>
#include <rtc_base/ssl_adapter.h>

#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/jsep.h>
#include <api/stats/rtcstats_objects.h>
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_factory.h>
#include <pc/video_track_source.h>
#include <rtc_base/thread.h>

#include <pc/session_description.h>

#include "test/vcm_capturer.h"
#include "test/test_video_capturer.h"

#include "fake_adm.h"
#include "tunnel_loggin.h"
#include "y4msource.h"
  

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> Peerconnection::_pcf = nullptr;
std::unique_ptr<rtc::Thread> Peerconnection::_signaling_th = nullptr;

constexpr int puissance(int v, unsigned int e)
{
  if(e == 0) return 1;
  return puissance(v, e - 1) * v;
}

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> Peerconnection::get_pcf()
{
  if(_pcf != nullptr) return _pcf;
  
  rtc::InitRandom((int)rtc::Time());
  rtc::InitializeSSL();
  
  _signaling_th = rtc::Thread::Create();
  _signaling_th->SetName("WebRTCSignalingThread", nullptr);
  _signaling_th->Start();
  
  rtc::scoped_refptr<FakeAudioDeviceModule> adm(new rtc::RefCountedObject<FakeAudioDeviceModule>());
  
  _pcf = webrtc::CreatePeerConnectionFactory(nullptr, nullptr, _signaling_th.get(), adm,
  webrtc::CreateBuiltinAudioEncoderFactory(),
  webrtc::CreateBuiltinAudioDecoderFactory(),
  webrtc::CreateBuiltinVideoEncoderFactory(),
  webrtc::CreateBuiltinVideoDecoderFactory(),
  nullptr, nullptr);
  
  webrtc::PeerConnectionFactoryInterface::Options options;
  options.crypto_options.srtp.enable_gcm_crypto_suites = true;
  _pcf->SetOptions(options);
  
  return _pcf;
}

void Peerconnection::clean()
{
  _pcf = nullptr;
  rtc::CleanupSSL();
}

Peerconnection::Peerconnection(rtc::scoped_refptr<SessionDescriptionObserverInterface> obs) : 
_pc{nullptr}, _obs(obs)
{
}

Peerconnection::~Peerconnection()
{
}

void Peerconnection::create_pc()
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "Start peerconnection";
  
  auto pcf = get_pcf();
  
  webrtc::PeerConnectionDependencies deps(this);
  webrtc::PeerConnectionInterface::RTCConfiguration config(webrtc::PeerConnectionInterface::RTCConfigurationType::kAggressive);
  
  config.set_cpu_adaptation(true);
  config.combined_audio_video_bwe.emplace(true);
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  
  if(port_range.has_value()) {
    config.set_min_port(port_range->min_port);
    config.set_max_port(port_range->max_port);
  }
  // constexpr const unsigned char magic_value[] = { 0xca, 0xfe, 0xba, 0xbe };

  auto res = pcf->CreatePeerConnectionOrError(config, std::move(deps));
  
  if(!res.ok()) {
    TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Can't create peerconnection : " << res.error().message();
    throw std::runtime_error("Could not create peerconection");
  }
  
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "Peerconnection created";

  _pc = res.value();
}

void Peerconnection::publish()
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "Peerconnection::publish";
  create_pc();
  
  webrtc::RtpTransceiverInit init;
  
  init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
  init.stream_ids = { "publish" };

  webrtc::RtpEncodingParameters encoding;
  encoding.rid = "h";
  encoding.active = true;
  encoding.max_bitrate_bps = 2'500'000;
  encoding.scale_resolution_down_by = 1.0;
  init.send_encodings.push_back(encoding);

  encoding.rid = "m";
  encoding.active = true;
  encoding.max_bitrate_bps = 1'200'000;
  encoding.scale_resolution_down_by = 2.0;
  init.send_encodings.push_back(encoding);

  encoding.rid = "l";
  encoding.active = true;
  encoding.max_bitrate_bps = 800'000;
  encoding.scale_resolution_down_by = 4.0;
  init.send_encodings.push_back(encoding);

  rtc::scoped_refptr<Y4MSource> capturer = rtc::make_ref_counted<Y4MSource>("file.y4m");
  if(!capturer) {
    TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Error creating capturer";
    throw std::runtime_error("Could not create capturer");
  }

  rtc::scoped_refptr<webrtc::VideoTrackInterface> track = _pcf->CreateVideoTrack("video", capturer.get());
  if(!track) {
    TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Error creating track";
    throw std::runtime_error("Could not create track");
  }

  track->AddOrUpdateSink(video_sink, rtc::VideoSinkWants());

  capturer->start();

  auto expected_transceiver = _pc->AddTransceiver(track, init);

  if(!expected_transceiver.ok()) {
    TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Error creating transceiver";
    throw std::runtime_error("Could not create transceiver");
  }

  _signaling_th->BlockingCall([this, track]() {
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "creating offer";
    _pc->CreateOffer(this, {});
  });
}

void Peerconnection::subscribe()
{
  create_pc();

  webrtc::RtpTransceiverInit init;
  
  init.direction = webrtc::RtpTransceiverDirection::kRecvOnly;
  init.stream_ids = { "sub" };

  auto expected_transceiver = _pc->AddTransceiver(cricket::MediaType::MEDIA_TYPE_VIDEO, init);
  if(!expected_transceiver.ok()) {
    TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Error creating transceiver " << expected_transceiver.error().message();
    return;
  }
  
  _signaling_th->BlockingCall([this]() {
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "creating offer";
    _pc->CreateOffer(this, {});
  });
}

void Peerconnection::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "On create offer success";
  
  _pc->SetLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface>(desc), _obs);
  
}

void Peerconnection::OnFailure(webrtc::RTCError error)
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "On create offer failure";
}

void Peerconnection::get_local_description(std::string &sdp)
{
  auto desc = _pc->local_description();

  if(!desc) {
    TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Error getting local sdp" << "\n";
    throw std::runtime_error("Could not get local description");
  }
  
  desc->ToString(&sdp);
}

void Peerconnection::stop()
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "PeerConnection::Stop" << "\n";
  
  _pc->Close();
  _pc = nullptr;
}

void Peerconnection::set_remote_description(const std::string &sdp)
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "Set remote desc";
  auto desc = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
  
  if(!desc) {
    TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Error parsing remote sdp" << "\n";
    throw std::runtime_error("Could not set remote description");
  }
  
  _pc->SetRemoteDescription(std::move(desc), _obs);
}

void Peerconnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) 
{
  switch(new_state) {
    case webrtc::PeerConnectionInterface::SignalingState::kClosed:
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "pc on closed";
    break;
    case webrtc::PeerConnectionInterface::SignalingState::kStable:
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "pc on stable";
    default:
    break;
  }
}

void Peerconnection::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) 
{}

void Peerconnection::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) 
{}

void Peerconnection::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver, const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) 
{}

void Peerconnection::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) 
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "PeerconnectionMgr::OnTrack";
  
  if(video_sink) {
    auto track = static_cast<webrtc::VideoTrackInterface*>(transceiver->receiver()->track().get());
    track->AddOrUpdateSink(video_sink, rtc::VideoSinkWants{});
  }
  
  // transceiver->receiver()->SetDepacketizerToDecoderFrameTransformer(_me);
}

void Peerconnection::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) 
{}

void Peerconnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)  
{}

void Peerconnection::OnRenegotiationNeeded() 
{}

void Peerconnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) 
{}

void Peerconnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) 
{}

void Peerconnection::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) 
{}

void Peerconnection::OnIceConnectionReceivingChange(bool receiving) 
{}

