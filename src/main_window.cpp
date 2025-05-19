#include <QGridLayout>

#include "main_window.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto* centralWidget = new QWidget(this);
    
    auto* layout = new QGridLayout(this);
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 3; ++j) {
            auto* videoWidget = new VideoFrameWidget(centralWidget);
            layout->addWidget(videoWidget, i, j);
            _video_widgets.push_back(videoWidget);
        }
    }

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    show();
}

MainWindow::~MainWindow()
{

}

VideoFrameWidget* MainWindow::use_video_widget(const std::string& id)
{
    if(_video_widgets.empty()) return nullptr;

    auto widget = _video_widgets.front();
    _video_widgets.pop_front();

    _video_widgets_used[id] = widget;

    return widget;
}

void MainWindow::release_video_widget(const std::string& id)
{
    auto widget = _video_widgets_used[id];
    _video_widgets.push_front(widget);
    _video_widgets_used.erase(id);
}