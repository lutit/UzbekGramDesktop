#pragma once

#include <QtCore/QString>

#include <functional>

namespace Window {
class SessionController;
} // namespace Window

namespace Ayu::UzbekVerification {

[[nodiscard]] bool Passed();
[[nodiscard]] QString MenuLabel();
void StartFlow(
	Window::SessionController *controller,
	std::function<void()> onChanged = {});

} // namespace Ayu::UzbekVerification
