#ifndef MEDOOZE_MGR_H
#define MEDOOZE_MGR_H

#include <condition_variable>

#include "websocket.h"

class MedoozeMgr
{
  WebSocketSecure _ws;

  std::condition_variable _cv;
  std::mutex              _cv_mutex;

  std::atomic_bool _alive;
  
public:

  using json = nlohmann::json;
  
  std::string host;

  int port;
  std::function<void(const std::string&, const std::string&)> onanswer;
  std::function<void(int, std::string&&)> ontarget;
  std::function<void(const std::string&)> onname;
  std::function<void()> onclose;
  std::function<void()> oncreate;
  std::function<void(const std::string& id)> onjoined;
  std::function<void(const std::string& id)> onnewparticipant;
  std::function<void(const std::string& id)> onnewpublisher;
  std::function<void(const std::string& id)> onremoveparticipant;
  
public:
  
  MedoozeMgr();

  void start();
  void stop();
  void view(const std::string& id, const std::string& sdp);
  void publish(const std::string& sdp);
};

#endif /* MEDOOZE_MGR_H */
