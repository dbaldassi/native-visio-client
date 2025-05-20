#ifndef WIDGET_H
#define WIDGET_H

#include <api/media_stream_interface.h>
#include <api/video/video_sink_interface.h>

#ifndef FAKE_RENDERER

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <mutex>

class VideoFrameWidget : public QOpenGLWidget,
                         protected QOpenGLFunctions,
                         public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
  Q_OBJECT

public:
  explicit VideoFrameWidget(QWidget * parent = nullptr);
  ~VideoFrameWidget();

  void OnFrame(const webrtc::VideoFrame& frame) override;

protected:
  void paintEvent(QPaintEvent*) override;

private:
  std::mutex _mutex;
  uint8_t*   _buffer;
  size_t     _buffer_size;
  int        _width;
  int        _height;
};

#else

class VideoFrameWidget : public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
  explicit VideoFrameWidget() = default;
  ~VideoFrameWidget() = default;

  void OnFrame(const webrtc::VideoFrame& frame) override {}
};

#endif

#endif // WIDGET_H