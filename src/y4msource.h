#ifndef Y4M_SOURCE_H
#define Y4M_SOURCE_H

#include <fstream>
#include <thread>
#include <string_view>
#include <optional>

#include <pc/video_track_source.h>
#include <modules/video_capture/video_capture.h>
#include <media/base/adapted_video_track_source.h>

class Y4MReader
{
    std::ifstream _file;
    int _width;
    int _height;
    int _fps;
    int _frame_size;

    std::streampos _header_end_pos;

    void parse_header();

public:
    explicit Y4MReader(std::string_view filename);
    ~Y4MReader();
    bool read_frame(std::vector<uint8_t>& frame_buffer);

    int width() const { return _width; }
    int height() const { return _height; }
    int fps() const { return _fps; }
};

class Y4MSource : public rtc::AdaptedVideoTrackSource // , public webrtc::VideoSinkInterface<webrtc::VideoFrame>
{
    std::jthread _thread;
    Y4MReader _reader;
    // Source state
    SourceState _state;

    void send_frames(std::stop_token stop_token);
public:
    explicit Y4MSource(std::string_view filename);

    SourceState state() const override { return _state; }
    absl::optional<bool> needs_denoising() const override { return false; }
    bool is_screencast() const override { return false; }
    bool remote() const override { return false; }

    void start();
    ~Y4MSource() override;
};

#endif /* Y4M_SOURCE_H */