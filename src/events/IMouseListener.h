#pragma once

class IMouseListener
{
public:
    virtual ~IMouseListener() = default;

    virtual void HandleMouseMove(double /*xpos*/, double /*ypos*/) {};
    virtual void HandleMouseButton(int /*button*/, int /*action*/) {};
    virtual void HandleMouseScroll(double /*xoffset*/, double /*yoffset*/) {};
};