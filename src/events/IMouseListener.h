#pragma once
#ifndef IMouseListener_h__
#define IMouseListener_h__

class IMouseListener
{
public:
    virtual void handleMouseMove(double /*xpos*/, double /*ypos*/) {};
    virtual void handleMouseButton(int /*button*/, int /*action*/) {};
    virtual void handleMouseScroll(double /*xoffset*/, double /*yoffset*/) {};
};

#endif // IMouseListener_h__
