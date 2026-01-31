// This is the source code of UzbekGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @UzbekGram, 2025
#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <rpl/producer.h>
#include <rpl/variable.h>

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace Ui {
class GenericBox;
class BoxContent;
} // namespace Ui

namespace Ayu {

class ShalavaPro : public QObject {
	Q_OBJECT

public:
	static ShalavaPro &instance();

	// Channel to subscribe: @uzbekgram_client
	static constexpr auto kRequiredChannel = "uzbekgram_client";

	// Check if user is subscribed to the required channel
	void checkSubscription(not_null<Main::Session*> session);

	// Returns true if PRO features are unlocked
	[[nodiscard]] bool isUnlocked() const;

	// Reactive producer for unlock state
	[[nodiscard]] rpl::producer<bool> unlockedValue() const;

	// Show welcome popup for first-time users
	void showWelcomePopup(not_null<Window::SessionController*> controller);

	// Show unlock popup (subscribe to channel)
	void showUnlockPopup(not_null<Window::SessionController*> controller);

	// Mark welcome popup as shown
	void markWelcomeShown();

	// Check if welcome was already shown
	[[nodiscard]] bool wasWelcomeShown() const;

	// Force refresh subscription status
	void refreshStatus(not_null<Main::Session*> session);

	// Open the required channel
	void openRequiredChannel(not_null<Window::SessionController*> controller);

private:
	ShalavaPro();

	void loadState();
	void saveState();

	void setUnlocked(bool unlocked);

	rpl::variable<bool> _unlocked = false;
	bool _welcomeShown = false;
	bool _checking = false;
};

} // namespace Ayu
