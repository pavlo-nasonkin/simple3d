#include "EventDispatcher.h"

EventDispatcher::EventDispatcher()
{

}

listenerFuncPtr EventDispatcher::addEventListener(const std::string& event, listenerFuncPtr handler)
{
    auto mapIterator = _listeners.find(event);
    if (mapIterator == _listeners.end()) {
        _listeners.insert({event, {handler}});
    }
    else
    {
        //prevent double insertion
        auto& vec = mapIterator->second;
        auto found = std::find(vec.begin(), vec.end(), handler) != vec.end();
        if (!found)
        {
            mapIterator->second.push_back(handler);
        }
    }
    return handler;
}

void EventDispatcher::removeEventListener(const std::string& event, listenerFuncPtr handler)
{
    auto mapIterator = _listeners.find(event);
    if (mapIterator != _listeners.end()) {
        auto& vec = mapIterator->second;
        auto found = std::find(vec.begin(), vec.end(), handler);
        if (found != vec.end())
        {
            vec.erase(found);
        }
        if (vec.size() == 0)
        {
            _listeners.erase(event);
        }
    }
}

void EventDispatcher::dispatchEvent(const std::string& event)
{
    auto mapIterator = _listeners.find(event);
    if (mapIterator != _listeners.end()) {
        auto listeners = mapIterator->second;
        for (auto listener : listeners)
        {
            (*listener)();
        }
    }
}

bool EventDispatcher::hasEventListener(const std::string& event)
{
    auto mapIterator = _listeners.find(event);
    return mapIterator != _listeners.end();
}
