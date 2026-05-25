#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct EventData
{

};
//#define ADD_EVENT_LISTENER(dispatcher, event, handlerFunc) dispatcher.addEventListener(event, [this](){handlerFunc();}, this, #handlerFunc)

typedef std::function<void()> listenerFunc;
typedef std::shared_ptr<listenerFunc> listenerFuncPtr;
#define WRAP_EVENT_HANDLER(eventHandler) std::make_shared<listenerFunc>([this](){eventHandler();})
class EventDispatcher
{
private:
    std::map<std::string, std::vector<listenerFuncPtr>, std::less<>> _listeners;
public:
    EventDispatcher();
    virtual ~EventDispatcher() = default;
    listenerFuncPtr addEventListener(std::string_view event, listenerFuncPtr handlerFunc);
    void removeEventListener(std::string_view event, listenerFuncPtr handlerFunc);
    void dispatchEvent(std::string_view event);
    bool hasEventListener(std::string_view event);
};

#endif // EVENTDISPATCHER_H
