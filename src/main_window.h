#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#ifndef FAKE_RENDERER

#include <deque>

#include <QMainWindow>
#include <QApplication>

#include "widget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

    VideoFrameWidget* use_video_widget(const std::string& id);
    void release_video_widget(const std::string& id);
    
private:
  std::unordered_map<std::string, VideoFrameWidget*> _video_widgets_used;
  std::deque<VideoFrameWidget*> _video_widgets;
};

#else

class MainWindow
{

public:
  explicit MainWindow() : _video_widgets(std::make_unique<VideoFrameWidget>()) {}
  ~MainWindow() = default;

    VideoFrameWidget* use_video_widget(const std::string&) { return _video_widgets.get(); }
    void release_video_widget(const std::string&) {}
    void close() {}

    void run() {}
    
private:
    std::unique_ptr<VideoFrameWidget> _video_widgets;
    std::condition_variable cv;
};

#endif

#endif // MAIN_WINDOW_H