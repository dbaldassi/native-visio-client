#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <mutex>

#include <api/media_stream_interface.h>
#include <api/video/video_sink_interface.h>

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


#endif // WIDGET_H