#include <iostream>
#include <csignal>
#include <thread>
#include <cstdlib>
#include <unistd.h>
#include <print>

#include "main_window.h"

#include "medooze_mgr.h"
#include "peerconnection_manager.h"
#include "monitor_manager.h"

#include "tunnel_loggin.h"
// #include "main_wnd.h"

class RTCSignalingObserver : public RTCSignalingObserverInterface
{
  MedoozeMgr& _ws;
public:
    RTCSignalingObserver(MedoozeMgr& ws) : _ws(ws) {}
    ~RTCSignalingObserver() = default;

    void on_local_description(const std::string& sdp) override
    {
        _ws.publish(sdp);
    }

    void on_remote_description() override
    {
        std::cout << "Remote description" << std::endl;
    }

    void on_local_description(const std::string& id, const std::string& sdp) override
    {
        std::cout << "Local description for " << id  << std::endl;
        _ws.view(id, sdp);
    }

    void on_remote_description(const std::string& id) override
    {
        std::println("Remote description for {}: ", id);
    }
};

int run(int argc, char * argv[])
{
#ifndef FAKE_RENDERER
  QApplication app(argc, argv);
#endif

  MedoozeMgr        medooze;
  PeerconnectionManager rtc_manager(std::make_shared<RTCSignalingObserver>(medooze));
  MonitorMgr        monitor;
  MainWindow        window;

  std::optional<std::string> name;

  if(argc == 2) {
    name = argv[1];
  }
  
  medooze.host = std::getenv("MEDOOZE_HOST");
  medooze.port = std::atoi(std::getenv("MEDOOZE_PORT"));

  monitor.host = std::getenv("MONITOR_HOST");
  monitor.port = std::atoi(std::getenv("MONITOR_PORT"));

  std::cout << "Medooze on : " << medooze.host << ":" << medooze.port << "\n";
  std::cout << "Monitor on : " << monitor.host << ":" << monitor.port << "\n";
  
  // pc.video_sink = &window;
  // pc.port_range = std::move(range);

  medooze.onname = [&monitor, &name](const std::string& mname) {
    // monitor.name = name.value_or(mname);
    // monitor.start();
  };
  medooze.onanswer = [&rtc_manager](const std::string& id, const std::string& sdp) -> void { 
    std::print("Answer: {}\n", id);
    rtc_manager.handle_answer(id, sdp);
  };
  medooze.onjoined = [&rtc_manager, &window](const std::string& id) -> void { 
    std::print("Joined: {}\n", id);
    rtc_manager.publish(id, window.use_video_widget(id));
  };
  medooze.onnewparticipant = [](const std::string& id) -> void { 
    std::print("New participant: {}\n", id);
  };
  medooze.onnewpublisher = [&rtc_manager, &window](const std::string& id) -> void { 
    std::print("New publisher: {}\n", id);
    rtc_manager.subscribe(id, window.use_video_widget(id));
  };
  medooze.onremoveparticipant = [&rtc_manager, &window](const std::string& id) -> void { 
    std::print("Removed participant: {}\n", id);
    rtc_manager.stop(id);
    window.release_video_widget(id);
  };

  medooze.onclose = [&window]() -> void { window.close(); };
  monitor.onclose = [&window]() -> void { window.close(); };

  // monitor.connect();
  medooze.start();
  int code;

#ifndef FAKE_RENDERER
  code = app.exec();
#else
  code = window.run();
#endif

  rtc_manager.stop();

  return code;
}

int main(int argc, char *argv[])
{
  TunnelLogging::set_min_severity(TunnelLogging::Severity::VERBOSE);

  int code = run(argc, argv);

  return code;
}
