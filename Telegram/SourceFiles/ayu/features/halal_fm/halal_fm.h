#pragma once

#include <QtCore/QString>

namespace Window {
class SessionController;
} // namespace Window

namespace Ayu::HalalFm {

[[nodiscard]] bool Enabled();
[[nodiscard]] QString Label();

void Toggle(Window::SessionController *controller);
void EnsureStartupPopup(Window::SessionController *controller);
void EnsureOverlay(Window::SessionController *controller);
void RemoveOverlay();

} // namespace Ayu::HalalFm
