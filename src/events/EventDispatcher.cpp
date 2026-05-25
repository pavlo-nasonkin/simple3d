#include "EventDispatcher.h"

EventDispatcher::EventDispatcher()
{

}

listenerFuncPtr EventDispatcher::addEventListener(std::string_view event, listenerFuncPtr handler)
{
    if (auto it = _listeners.find(event); it != _listeners.end()) {
        auto& vec = it->second;
        if (std::find(vec.begin(), vec.end(), handler) == vec.end()) {
            vec.push_back(handler);
        }
    } else {
        _listeners.emplace(std::string(event), std::vector<listenerFuncPtr>{handler});
    }
    return handler;
}

void EventDispatcher::removeEventListener(std::string_view event, listenerFuncPtr handler)
{
    auto mapIterator = _listeners.find(event);
    if (mapIterator == _listeners.end()) return;

    auto& vec = mapIterator->second;
    auto found = std::find(vec.begin(), vec.end(), handler);
    if (found != vec.end()) {
        vec.erase(found);
    }
    if (vec.empty()) {
        _listeners.erase(mapIterator);
    }
}

void EventDispatcher::dispatchEvent(std::string_view event)
{
    auto it = _listeners.find(event);
    if (it == _listeners.end()) return;

    auto listeners = it->second;
    for (auto& listener : listeners) {
        (*listener)();
    };
}

bool EventDispatcher::hasEventListener(std::string_view event)
{
    return _listeners.find(event) != _listeners.end();
}
