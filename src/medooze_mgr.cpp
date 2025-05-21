#include <iostream>
#include "medooze_mgr.h"

#include "tunnel_loggin.h"

MedoozeMgr::MedoozeMgr() : _alive{false}
{
  _ws.onopen = [this]()
  {
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "medooze ws opened";
    _alive = true;
    _cv.notify_all();

    // join room
    _ws.send(json{{"cmd", "join"}, {"roomId", room }}.dump());
  };
  _ws.onclose = [this]()
  {
    TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "medooze ws closed";
    _alive = false;
    if (onclose)
      onclose();
  };
}

void MedoozeMgr::start()
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "Start connection medooze manager";

  _ws.connect(host, port, "vm-visio");
  _ws.onmessage = [this](auto &&msg)
  {
     auto cmd = msg["cmd"].template get<std::string>();

    if(auto it = msg.find("success"); it != msg.end()) {
      bool success = it->template get<bool>();
      if(!success) {
        TUNNEL_LOG(TunnelLogging::Severity::ERROR) << "MedoozeManager: error in command " << cmd;
        return;
      }
    }

     if(cmd == "answer" && onanswer) onanswer(msg["id"].template get<std::string>(), msg["answer"].template get<std::string>());
     else if (cmd == "target" && ontarget) ontarget(msg["target"].template get<int>(), msg["rid"].template get<std::string>());
     else if (cmd == "name" && onname) onname(msg["name"].template get<std::string>());
     else if (cmd == "create" && oncreate) oncreate();
     else if (cmd == "join" && onjoined) onjoined(msg["id"].template get<std::string>());
     else if (cmd == "new_participant" && onnewparticipant) onnewparticipant(msg["id"].template get<std::string>());
     else if (cmd == "new_publisher" && onnewpublisher) onnewpublisher(msg["id"].template get<std::string>());
     else if (cmd == "remove_participant" && onremoveparticipant) onremoveparticipant(msg["id"].template get<std::string>());
  };
}

void MedoozeMgr::stop()
{
  _ws.disconnect();
}

void MedoozeMgr::view(const std::string& id, const std::string &sdp)
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "MedoozeManager::view";

  if (!_alive)
  {
    std::unique_lock<std::mutex> lock(_cv_mutex);
    _cv.wait(lock);
  }

  json cmd = {
      {"cmd", "view"},
      {"offer", sdp},
      {"remoteId", id},
    };

  _ws.send(cmd.dump());
}

void MedoozeMgr::publish(const std::string &sdp)
{
  TUNNEL_LOG(TunnelLogging::Severity::VERBOSE) << "MedoozeManager::publish";

  if (!_alive)
  {
    std::unique_lock<std::mutex> lock(_cv_mutex);
    _cv.wait(lock);
  }

  json cmd = {
      {"cmd", "publish"},
      {"offer", sdp},
    };

  _ws.send(cmd.dump());
}