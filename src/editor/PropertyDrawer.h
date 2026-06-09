#pragma once

class Behaviour;
class Pivot3D;

namespace editor {

// Рисует редактируемые поля компонента (из Behaviour::GetProperties) виджетами ImGui.
// contextNode нужен для резолва ссылок (NodeRef/BehRef) по id через корень иерархии и
// для приёма drag-n-drop полезной нагрузки "NODE_ID" из панели Hierarchy.
void DrawBehaviourProperties(Behaviour* behaviour, Pivot3D* contextNode);

} // namespace editor
