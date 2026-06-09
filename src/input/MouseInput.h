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

	// Текущее состояние кнопки (для поллинга из Behaviour). button — код GLFW (0..7).
	bool IsButtonPressed(int button) const;

	// Накопленный вертикальный скролл с прошлого вызова (для поллинга из Behaviour).
	double ConsumeScrollY();

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
	static constexpr int kMaxButtons = 8;
	bool _buttons[kMaxButtons] = {};
	double _scrollAccumY = 0.0;
private:
	std::vector<IMouseListener*>* GetContainerByType(const std::string& type);

	std::vector<IMouseListener*> _cursorPosListeners;
	std::vector<IMouseListener*> _mouseButtonListeners;
	std::vector<IMouseListener*> _scrollListeners;
};