#pragma once

#include <string>
#include <vector>
class IMouseListener;

class MouseInput
{
public:
	static const std::string MOUSE_MOVE;
	static const std::string MOUSE_BUTTON;
	static const std::string MOUSE_SCROLL;

	MouseInput();
	~MouseInput() = default;
	void AddListener(IMouseListener* listener, const std::string& type);
	void RemoveListener(IMouseListener* listener, const std::string& type);
	double GetMouseX() const { return _mouseX; };
	double GetMouseY() const { return _mouseY; };

	void OnMouseMove(double xPos, double yPos);
	void OnMouseButton(int button, int action);
	void OnMouseScroll(double xOffset, double yOffset);

	// Управление курсором (реализуется бэкендом ввода, напр. GLFW).
	virtual void SetCursorPosition(double x, double y) {}
	virtual void SetCursorVisible(bool visible) {}

	// Когда UI (ImGui) забирает мышь — события не рассылаются слушателям (камере/пикингу).
	void SetMouseCaptured(bool captured) { _mouseCaptured = captured; }

protected:
	double _mouseX = 0.0;
	double _mouseY = 0.0;
	bool _mouseCaptured = false;
private:
	std::vector<IMouseListener*>* GetContainerByType(const std::string& type);

	std::vector<IMouseListener*> _cursorPosListeners;
	std::vector<IMouseListener*> _mouseButtonListeners;
	std::vector<IMouseListener*> _scrollListeners;
};