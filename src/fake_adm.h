#ifndef FAKE_AUDIO_DEVICE_MODULE_H
#define FAKE_AUDIO_DEVICE_MODULE_H

#include <api/scoped_refptr.h>
#include <modules/audio_device/include/audio_device.h>

class FakeAudioDeviceModule : public webrtc::AudioDeviceModule
{  
  int32_t ActiveAudioLayer(AudioLayer*) const override { return 0; }

  // Note: Calling this method from a callback may result in deadlock.
  int32_t RegisterAudioCallback(webrtc::AudioTransport* audio_callback) override { return 0; }

  int32_t Init() override { return 0; }
  int32_t Terminate() override { return 0;  }
  bool Initialized() const override { return true; }

  int16_t PlayoutDevices() override { return 0; }
  int16_t RecordingDevices() override { return 0; }
  int32_t PlayoutDeviceName(uint16_t index,
			    char name[webrtc::kAdmMaxDeviceNameSize],
			    char guid[webrtc::kAdmMaxGuidSize]) override { return 0; }
  int32_t RecordingDeviceName(uint16_t index,
			      char name[webrtc::kAdmMaxDeviceNameSize],
			      char guid[webrtc::kAdmMaxGuidSize]) override { return 0; }

  int32_t SetPlayoutDevice(uint16_t index) override { return 0; }
  int32_t SetPlayoutDevice(WindowsDeviceType device) override { return 0; }
  int32_t SetRecordingDevice(uint16_t index) override { return 0; }
  int32_t SetRecordingDevice(WindowsDeviceType device) override { return 0; }

  int32_t PlayoutIsAvailable(bool*) override { return 0; }
  int32_t InitPlayout() override { return 0; }
  bool PlayoutIsInitialized() const override { return true; }
  int32_t RecordingIsAvailable(bool*) override { return 0; }
  int32_t InitRecording() override { return 0; }
  bool RecordingIsInitialized() const override { return true; }

  int32_t StartPlayout() override { return 0; }
  int32_t StopPlayout() override { return 0; }
  bool Playing() const override { return true; }
  int32_t StartRecording() override { return 0; }
  int32_t StopRecording() override { return 0; }
  bool Recording() const override { return true; }

  int32_t InitSpeaker() override { return 0; }
  bool SpeakerIsInitialized() const override { return true; }
  int32_t InitMicrophone() override { return 0; }
  bool MicrophoneIsInitialized() const override { return true; }

  int32_t SpeakerVolumeIsAvailable(bool*) override { return 0; }
  int32_t SetSpeakerVolume(uint32_t) override { return 0; }
  int32_t SpeakerVolume(uint32_t*) const override { return 0; }
  int32_t MaxSpeakerVolume(uint32_t*) const override { return 0; }
  int32_t MinSpeakerVolume(uint32_t*) const override { return 0; }

  int32_t MicrophoneVolumeIsAvailable(bool*) override { return 0; }
  int32_t SetMicrophoneVolume(uint32_t volume) override { return 0; }
  int32_t MicrophoneVolume(uint32_t* volume) const override { return 0; }
  int32_t MaxMicrophoneVolume(uint32_t* max_volume) const override { return 0; }

  int32_t MinMicrophoneVolume(uint32_t*) const override { return 0; }

  int32_t SpeakerMuteIsAvailable(bool*) override { return 0; }
  int32_t SetSpeakerMute(bool) override { return 0; }
  int32_t SpeakerMute(bool*) const override { return 0; }

  int32_t MicrophoneMuteIsAvailable(bool*) override { return 0; }
  int32_t SetMicrophoneMute(bool) override { return 0; }
  int32_t MicrophoneMute(bool*) const override { return 0; }

  int32_t StereoPlayoutIsAvailable(bool*) const override { return 0; }
  int32_t SetStereoPlayout(bool) override { return 0; }
  int32_t StereoPlayout(bool*) const override { return 0; }
  int32_t StereoRecordingIsAvailable(bool*) const override { return 0; }
  int32_t SetStereoRecording(bool) override { return 0; }
  int32_t StereoRecording(bool*) const override { return 0; }

  int32_t PlayoutDelay(uint16_t* delay_ms) const override { return 0; }

  bool BuiltInAECIsAvailable() const override { return false; }
  int32_t EnableBuiltInAEC(bool) override { return -1; }
  bool BuiltInAGCIsAvailable() const override { return false; }
  int32_t EnableBuiltInAGC(bool) override { return -1; }
  bool BuiltInNSIsAvailable() const override { return false; }
  int32_t EnableBuiltInNS(bool) override { return -1; }

  int32_t GetPlayoutUnderrunCount() const override { return -1; }


protected:
  FakeAudioDeviceModule() {}
  ~FakeAudioDeviceModule() {}
};

#endif
