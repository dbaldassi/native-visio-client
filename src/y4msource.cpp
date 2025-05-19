#include <api/video/i420_buffer.h>
#include <print>
#include <chrono>

#include "y4msource.h"
#include "tunnel_loggin.h"


Y4MReader::Y4MReader(std::string_view filename) : _file(filename.data())
{
    if (!_file.is_open())
    {
        TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Failed to open file: " << filename;
        throw std::runtime_error("Failed to open file");
    }

    parse_header();
}

Y4MReader::~Y4MReader()
{
    if (_file.is_open())
    {
        _file.close();
    }
}

void Y4MReader::parse_header()
{
    std::string header_line;
    std::getline(_file, header_line);

    if (header_line.substr(0, 9) != "YUV4MPEG2") {
        TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "Invalid Y4M header";
        throw std::runtime_error("Invalid Y4M header");
    }

    size_t w_pos = header_line.find(" W");
    size_t h_pos = header_line.find(" H");
    size_t f_pos = header_line.find(" F");

    _width = std::stoi(header_line.substr(w_pos + 2, h_pos - w_pos - 2));
    _height = std::stoi(header_line.substr(h_pos + 2, f_pos - h_pos - 2));
    _fps = std::stoi(header_line.substr(f_pos + 2, header_line.find(":", f_pos) - f_pos - 2));

    // Calculate frame size
    _frame_size = (_width * _height * 3) / 2; // YUV420P

    // Save the position of the end of the header
    _header_end_pos = _file.tellg();
}

bool Y4MReader::read_frame(std::vector<uint8_t>& frame_buffer)
{
    if (!_file.good()) {
        // Rewind to the beginning of the file if EOF is reached
        _file.clear(); // Clear EOF flag
        _file.seekg(_header_end_pos); // Seek to the end of the header
    }
    
    std::string frame_header;
    if (!std::getline(_file, frame_header) || frame_header.rfind("FRAME", 0) != 0) {
        return false; // Fin de fichier ou mauvais format
    }
    
    _file.read(reinterpret_cast<char*>(frame_buffer.data()), _frame_size);
    return _file.gcount() == _frame_size;
}

Y4MSource::Y4MSource(std::string_view filename) : rtc::AdaptedVideoTrackSource(), _reader(filename), _state(SourceState::kEnded)
{}
Y4MSource::~Y4MSource()
{}

void Y4MSource::start()
{
    if (_state == SourceState::kEnded) {
        _state = SourceState::kLive;
        _thread = std::jthread([this](std::stop_token stop_token) {
            _state = SourceState::kLive;
            send_frames(stop_token);
        });
    }
}

void Y4MSource::send_frames(std::stop_token stop_token)
{
    std::vector<uint8_t> frame_buffer(_reader.width() * _reader.height() * 3 / 2);
    const int frame_period_ms = 1000 / _reader.fps();

    while (!stop_token.stop_requested()) {
        auto start = std::chrono::steady_clock::now();
        _reader.read_frame(frame_buffer);
        auto frame = webrtc::VideoFrame::Builder()
                        .set_video_frame_buffer(webrtc::I420Buffer::Copy(
                            _reader.width(), _reader.height(),
                            frame_buffer.data(), _reader.width(),
                            frame_buffer.data() + _reader.width() * _reader.height(),
                            _reader.width() / 2,
                            frame_buffer.data() + _reader.width() * _reader.height() * 5 / 4,
                            _reader.width() / 2))
                        .set_timestamp_rtp(0)  // Set appropriate timestamp
                        .build();

        OnFrame(frame);
        
        auto end = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        int sleep_ms = frame_period_ms - static_cast<int>(elapsed_ms);
        if (sleep_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }

    std::println("Y4MSource: End of stream");
}