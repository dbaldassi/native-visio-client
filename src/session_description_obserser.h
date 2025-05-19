#ifndef SESSION_DESCRIPTION_OBSERVER_H
#define SESSION_DESCRIPTION_OBSERVER_H

#include <api/set_local_description_observer_interface.h>
#include <api/set_remote_description_observer_interface.h>

class SessionDescriptionObserverInterface : public webrtc::SetLocalDescriptionObserverInterface, 
public webrtc::SetRemoteDescriptionObserverInterface
{
public:
    void AddRef() const override = 0;
    rtc::RefCountReleaseStatus Release() const override  = 0;
};
#endif /* SESSION_DESCRIPTION_OBSERVER_H */