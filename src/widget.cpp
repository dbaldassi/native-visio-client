
#include <QOpenGLTexture>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QWidget>

#include <print>

#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "widget.h"

// #include <QSGSimpleTextureNode>

VideoFrameWidget::VideoFrameWidget(QWidget *parent) :  QOpenGLWidget(parent),
  _buffer(nullptr),
  _buffer_size(0)
{
  setMinimumSize(1,1);
}

VideoFrameWidget::~VideoFrameWidget()
{
  if(_buffer) delete [] _buffer;
}

void VideoFrameWidget::OnFrame(const webrtc::VideoFrame& frame)
{
    // std::println("OnFrame {}x{}", frame.width(), frame.height());

  std::lock_guard<std::mutex> lock(_mutex);
  size_t new_size = frame.width() * frame.height() * 4;


  if (new_size > _buffer_size) {
    _buffer_size = new_size;
    delete [] _buffer;
    _buffer = new uint8_t[_buffer_size];
  }

  webrtc::ConvertFromI420(frame, webrtc::VideoType::kARGB, 0, _buffer);

  // Get frame size for aspect ratio calculations in paint thread.
  _width  = frame.width();
  _height = frame.height();

  // Schedule painting
  update();
}

void VideoFrameWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);

  // Create image from the buffer
  QImage img = QImage(
    static_cast<uchar *>(_buffer),
    _width,_height,
    QImage::Format_ARGB32
  );

  // Aspect ratio calculation
  int newWidth = rect().width();
  if (_height != 0) newWidth = _width / (float)_height * rect().height();
  painter.drawImage(QRect(0, 0, newWidth, rect().height()), img);
}
