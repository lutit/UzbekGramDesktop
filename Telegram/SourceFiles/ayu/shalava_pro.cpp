// This is the source code of UzbekGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @UzbekGram, 2025
#define _USE_MATH_DEFINES
#include "ayu/shalava_pro.h"
#include "ayu/ayu_settings.h"

#include "main/main_session.h"
#include "data/data_session.h"
#include "data/data_channel.h"
#include "data/data_peer.h"
#include "window/window_session_controller.h"
#include "apiwrap.h"
#include "core/application.h"

#include "ui/layers/generic_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/gradient_round_button.h"
#include "ui/effects/premium_graphics.h"
#include "ui/effects/animations.h"
#include "ui/text/text_utilities.h"
#include "ui/painter.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"

#include "boxes/abstract_box.h"
#include "lang/lang_keys.h"

#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_premium.h"
#include "styles/style_settings.h"
#include "styles/style_info.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <cmath>

namespace Ayu {

namespace {

QString GetStatePath() {
	return QString::fromStdString(AyuSettings::getSettingsPath()) + "_shalava_pro.json";
}

class StarWidget : public Ui::RpWidget {
public:
	explicit StarWidget(QWidget *parent)
	: Ui::RpWidget(parent)
	, _animation([=](crl::time now) { update(); return true; }) {
		resize(120, 120);
		_animation.start();
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		p.setRenderHint(QPainter::Antialiasing);

		const auto size = std::min(width(), height());
		const auto center = QPointF(width() / 2.0, height() / 2.0);
		const auto now = crl::now();
		const auto rotation = (now % 10000) / 10000.0 * 360.0;
		const auto pulse = 0.9 + 0.1 * std::sin(now / 500.0 * M_PI);

		// Glow effect
		for (int i = 5; i > 0; --i) {
			const auto glowSize = size * pulse * (1.0 + i * 0.08);
			const auto alpha = 30 - i * 5;
			QRadialGradient glow(center, glowSize / 2);
			glow.setColorAt(0, QColor(255, 215, 0, alpha));
			glow.setColorAt(1, QColor(255, 140, 0, 0));
			p.setBrush(glow);
			p.setPen(Qt::NoPen);
			p.drawEllipse(center, glowSize / 2, glowSize / 2);
		}

		// Draw star
		p.save();
		p.translate(center);
		p.rotate(rotation);
		p.scale(pulse, pulse);

		// Premium gradient
		QLinearGradient gradient(-size/3, -size/3, size/3, size/3);
		gradient.setColorAt(0, QColor(255, 215, 0));   // Gold
		gradient.setColorAt(0.5, QColor(255, 165, 0)); // Orange
		gradient.setColorAt(1, QColor(255, 69, 0));    // Red-Orange

		QPainterPath starPath;
		const int points = 5;
		const double outerR = size / 3.0;
		const double innerR = outerR * 0.4;

		for (int i = 0; i < points * 2; ++i) {
			const double r = (i % 2 == 0) ? outerR : innerR;
			const double angle = M_PI / 2 + i * M_PI / points;
			const double x = r * std::cos(angle);
			const double y = -r * std::sin(angle);
			if (i == 0) {
				starPath.moveTo(x, y);
			} else {
				starPath.lineTo(x, y);
			}
		}
		starPath.closeSubpath();

		p.setBrush(gradient);
		p.setPen(QPen(QColor(255, 255, 255, 200), 2));
		p.drawPath(starPath);

		p.restore();
	}

private:
	Ui::Animations::Basic _animation;
};

class ShineWidget : public Ui::RpWidget {
public:
	explicit ShineWidget(QWidget *parent)
	: Ui::RpWidget(parent)
	, _animation([=](crl::time now) { update(); return true; }) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		_animation.start();
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		p.setRenderHint(QPainter::Antialiasing);

		const auto now = crl::now();
		const auto phase = (now % 3000) / 3000.0;

		// Horizontal shine
		const auto x = -width() * 0.3 + (width() * 1.6) * phase;

		QLinearGradient shine(x - 50, 0, x + 50, 0);
		shine.setColorAt(0, QColor(255, 255, 255, 0));
		shine.setColorAt(0.5, QColor(255, 255, 255, 100));
		shine.setColorAt(1, QColor(255, 255, 255, 0));

		p.fillRect(rect(), shine);
	}

private:
	Ui::Animations::Basic _animation;
};

} // namespace

ShalavaPro &ShalavaPro::instance() {
	static ShalavaPro inst;
	return inst;
}

ShalavaPro::ShalavaPro() {
	loadState();
}

void ShalavaPro::loadState() {
	QFile file(GetStatePath());
	if (!file.open(QIODevice::ReadOnly)) return;

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();
	_welcomeShown = obj.value("welcomeShown").toBool(false);
	_pornTvShown = obj.value("pornTvShown").toBool(false);
	_unlocked = obj.value("unlocked").toBool(false);
}

void ShalavaPro::saveState() {
	QJsonObject obj;
	obj["welcomeShown"] = _welcomeShown;
	obj["pornTvShown"] = _pornTvShown;
	obj["unlocked"] = _unlocked.current();
	QFile file(GetStatePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(obj).toJson());
	}
}

bool ShalavaPro::isUnlocked() const {
	return _unlocked.current();
}

rpl::producer<bool> ShalavaPro::unlockedValue() const {
	return _unlocked.value();
}

void ShalavaPro::setUnlocked(bool unlocked) {
	if (_unlocked.current() != unlocked) {
		_unlocked = unlocked;
		saveState();
	}
}

void ShalavaPro::markWelcomeShown() {
	_welcomeShown = true;
	saveState();
}

bool ShalavaPro::wasWelcomeShown() const {
	return _welcomeShown;
}

void ShalavaPro::markPornTvShown() {
	_pornTvShown = true;
	saveState();
}

bool ShalavaPro::wasPornTvShown() const {
	return _pornTvShown;
}

void ShalavaPro::checkSubscription(not_null<Main::Session*> session) {
	if (_checking) return;

	// Try to find the channel by username
	const auto peer = session->data().peerByUsername(QString::fromLatin1(kRequiredChannel));
	if (peer) {
		if (const auto channel = peer->asChannel()) {
			setUnlocked(channel->amIn());
			return;
		}
	}

	    // Channel not cached, need to resolve
	    _checking = true;
	    session->api().request(MTPcontacts_ResolveUsername(
	        MTP_flags(0),
	        MTP_string(kRequiredChannel),
	        MTP_string()
	    )).done([=](const MTPcontacts_ResolvedPeer &result) {
	        _checking = false;
	        result.match([&](const MTPDcontacts_resolvedPeer &data) {
	            session->data().processUsers(data.vusers());
	            session->data().processChats(data.vchats());
	
	            const auto peerId = peerFromMTP(data.vpeer());
	            if (const auto peer = session->data().peerLoaded(peerId)) {
	                if (const auto channel = peer->asChannel()) {
	                    setUnlocked(channel->amIn());
	                }
	            }
	        });
	    }).fail([=] {
	        _checking = false;
	    }).send();
	}
	
	void ShalavaPro::refreshStatus(not_null<Main::Session*> session) {
	    checkSubscription(session);
	}
	
	
	void ShalavaPro::openRequiredChannel(not_null<Window::SessionController*> controller) {
	    const auto session = &controller->session();
	    const auto peer = session->data().peerByUsername(QString::fromLatin1(kRequiredChannel));
	
	    if (peer) {
	        controller->showPeerHistory(peer);
	    } else {
	        // Resolve and open
	        session->api().request(MTPcontacts_ResolveUsername(
	            MTP_flags(0),
	            MTP_string(kRequiredChannel),
	            MTP_string()
	        )).done([=](const MTPcontacts_ResolvedPeer &result) {
	            result.match([&](const MTPDcontacts_resolvedPeer &data) {
	                session->data().processUsers(data.vusers());
	                session->data().processChats(data.vchats());
	                const auto peerId = peerFromMTP(data.vpeer());
	                if (const auto p = session->data().peerLoaded(peerId)) {
	                    controller->showPeerHistory(p);
	                }
	            });
	        }).send();
	    }
	}
	
	void ShalavaPro::showWelcomePopup(not_null<Window::SessionController*> controller) {
	    if (_welcomeShown) return;
	
	    auto box = Box([=](not_null<Ui::GenericBox*> box) {
	        box->setWidth(st::boxWideWidth);
	        box->setNoContentMargin(true);
	
	        // Premium-style gradient background at top
	        const auto top = box->addRow(
	            object_ptr<Ui::RpWidget>(box),
	            QMargins(0, 0, 0, 0));
	        top->resize(st::boxWideWidth, 200);
	        		top->paintRequest().start([=](auto) {
	        			auto p = QPainter(top);
	        			QLinearGradient gradient(0, 0, top->width(), top->height());
	        			gradient.setColorAt(0, QColor(106, 17, 203));
	        			gradient.setColorAt(0.5, QColor(186, 85, 211));
	        			gradient.setColorAt(1, QColor(255, 140, 0));
	        			p.fillRect(top->rect(), gradient);
	        		}, [](const auto &) {}, [] {}, top->lifetime());	
	        // Animated star
	        const auto star = Ui::CreateChild<StarWidget>(top);
	        star->move((top->width() - star->width()) / 2, 40);
	
	        // Shine effect
	        const auto shine = Ui::CreateChild<ShineWidget>(top);
	        shine->setGeometry(top->rect());
	        shine->raise();
	
	        // Title
	        box->addRow(
	            object_ptr<Ui::FlatLabel>(
	                box,
	                rpl::single(QString::fromUtf8("✨ SHALAVA PRO ДОСТУПЕН! ✨")),
	                st::boxTitle),
	            st::boxRowPadding + QMargins(0, 20, 0, 0));
	
	        box->addRow(
	            object_ptr<Ui::FlatLabel>(
	                box,
	                rpl::single(QString::fromUtf8(
	                    "Разблокируй эксклюзивные эффекты из Telegram Premium!\n\n"
	                    "🌟 Премиум анимации звёзд\n"
	                    "💫 Эффекты как в оригинальном TG\n"
	                    "🔥 Ультра режим с огнём\n"
	                    "✨ Специальные визуальные эффекты")),
	                st::boxLabel),
	            st::boxRowPadding);
	
	        box->addRow(
	            object_ptr<Ui::FlatLabel>(
	                box,
	                rpl::single(QString::fromUtf8(
	                    "Чтобы получить PRO, подпишись на канал:\n"
	                    "@uzbekgram_client")),
	                st::boxLabel),
	            st::boxRowPadding);
	
	        // Premium gradient button
	        auto button = object_ptr<Ui::GradientButton>(box, QGradientStops{
	            { 0.0, QColor(106, 17, 203) },
	            { 0.5, QColor(186, 85, 211) },
	            { 1.0, QColor(255, 140, 0) },
	        });
	        button->resize(st::boxWideWidth - st::boxRowPadding.left() - st::boxRowPadding.right(), st::settingsButton.height);
	        button->setClickedCallback([=, controller = controller] {
	            markWelcomeShown();
	            box->closeBox();
	            openRequiredChannel(controller);
	        });
	
	        // Button text
	        const auto buttonText = Ui::CreateChild<Ui::FlatLabel>(
	            button.data(),
	            rpl::single(QString::fromUtf8("ПОДПИСАТЬСЯ И ПОЛУЧИТЬ PRO")),
	            st::defaultFlatLabel);
	        buttonText->setAttribute(Qt::WA_TransparentForMouseEvents);
	        buttonText->setStyleSheet("color: white; font-weight: bold;");
	        		button->sizeValue().start([=](QSize size) {
	        			buttonText->move(
	        				(size.width() - buttonText->width()) / 2,
	        				(size.height() - buttonText->height()) / 2);
	        		}, [](const auto &) {}, [] {}, buttonText->lifetime());	
	        box->addRow(std::move(button), st::boxRowPadding + QMargins(0, 20, 0, 0));
	
	        box->addButton(tr::lng_close(), [=] {
	            markWelcomeShown();
	            box->closeBox();
	        });
	    });
	
	    controller->show(std::move(box));
	}
	
	void ShalavaPro::showPornTvPopup(not_null<Window::SessionController*> controller) {
		if (_pornTvShown) return;

		auto box = Box([=](not_null<Ui::GenericBox*> box) {
			box->setWidth(st::boxWideWidth);
			box->setNoContentMargin(true);

			// Premium-style gradient background at top
			const auto top = box->addRow(
				object_ptr<Ui::RpWidget>(box),
				QMargins(0, 0, 0, 0));
			top->resize(st::boxWideWidth, 200);
			top->paintRequest().start([=](auto) {
				auto p = QPainter(top);
				QLinearGradient gradient(0, 0, top->width(), top->height());
				gradient.setColorAt(0, QColor(255, 20, 147)); // DeepPink
				gradient.setColorAt(0.5, QColor(255, 69, 0)); // Red-Orange
				gradient.setColorAt(1, QColor(138, 43, 226)); // BlueViolet
				p.fillRect(top->rect(), gradient);
			}, [](const auto &) {}, [] {}, top->lifetime());

			// Animated star
			const auto star = Ui::CreateChild<StarWidget>(top);
			star->move((top->width() - star->width()) / 2, 40);

			// Shine effect
			const auto shine = Ui::CreateChild<ShineWidget>(top);
			shine->setGeometry(top->rect());
			shine->raise();

			// Title
			box->addRow(
				object_ptr<Ui::FlatLabel>(
					box,
					rpl::single(QString::fromUtf8("📺 NEW PORN TV FEATURE! 📺")),
					st::boxTitle),
				st::boxRowPadding + QMargins(0, 20, 0, 0));

			box->addRow(
				object_ptr<Ui::FlatLabel>(
					box,
					rpl::single(QString::fromUtf8(
						"Мы добавили новую кнопку в сайдбар!\n\n"
						"🎥 Бесконечный поток контента\n"
						"🚀 Быстрый доступ в один клик\n"
						"🙈 Никаких ссылок и переходов\n"
						"🔥 Просто нажми и наслаждайся")),
					st::boxLabel),
				st::boxRowPadding);

			// Close button
			auto button = object_ptr<Ui::GradientButton>(box, QGradientStops{
				{ 0.0, QColor(255, 20, 147) },
				{ 1.0, QColor(138, 43, 226) },
			});
			button->resize(st::boxWideWidth - st::boxRowPadding.left() - st::boxRowPadding.right(), st::settingsButton.height);
			button->setClickedCallback([=] {
				markPornTvShown();
				box->closeBox();
			});

			// Button text
			const auto buttonText = Ui::CreateChild<Ui::FlatLabel>(
				button.data(),
				rpl::single(QString::fromUtf8("ПОНЯТНО, СПАСИБО")),
				st::defaultFlatLabel);
			buttonText->setAttribute(Qt::WA_TransparentForMouseEvents);
			buttonText->setStyleSheet("color: white; font-weight: bold;");
			button->sizeValue().start([=](QSize size) {
				buttonText->move(
					(size.width() - buttonText->width()) / 2,
					(size.height() - buttonText->height()) / 2);
			}, [](const auto &) {}, [] {}, buttonText->lifetime());
			box->addRow(std::move(button), st::boxRowPadding + QMargins(0, 20, 0, 20));
		});

		controller->show(std::move(box));
	}

	void ShalavaPro::showUnlockPopup(not_null<Window::SessionController*> controller) {
	    // Refresh status first
	    refreshStatus(&controller->session());
	
	    auto box = Box([=](not_null<Ui::GenericBox*> box) {
	        box->setWidth(st::boxWideWidth);
	        box->setNoContentMargin(true);
	
	        // Gradient header
	        const auto top = box->addRow(
	            object_ptr<Ui::RpWidget>(box),
	            QMargins(0, 0, 0, 0));
	        top->resize(st::boxWideWidth, 160);
	        		top->paintRequest().start([=](auto) {
	        			auto p = QPainter(top);
	        			QLinearGradient gradient(0, 0, top->width(), top->height());
	        			gradient.setColorAt(0, QColor(255, 215, 0));
	        			gradient.setColorAt(0.5, QColor(255, 165, 0));
	        			gradient.setColorAt(1, QColor(255, 69, 0));
	        			p.fillRect(top->rect(), gradient);
	        		}, [](const auto &) {}, [] {}, top->lifetime());	
	        // Star
	        const auto star = Ui::CreateChild<StarWidget>(top);
	        star->resize(80, 80);
	        star->move((top->width() - star->width()) / 2, 40);
	
	        const auto unlocked = _unlocked.current();
	
	        // Title based on status
	        box->addRow(
	            object_ptr<Ui::FlatLabel>(
	                box,
	                rpl::single(unlocked
	                    ? QString::fromUtf8("✨ SHALAVA PRO АКТИВЕН!")
	                    : QString::fromUtf8("🔒 SHALAVA PRO")),
	                st::boxTitle),
	            st::boxRowPadding + QMargins(0, 20, 0, 0));
	
	        if (unlocked) {
	            box->addRow(
	                object_ptr<Ui::FlatLabel>(
	                    box,
	                    rpl::single(QString::fromUtf8(
	                        "Спасибо за подписку!\n\n"
	                        "Все PRO эффекты разблокированы:\n"
	                        "🌟 Премиум режим (кнопка 4)\n"
	                        "💫 Эффекты частиц из TG Premium\n"
	                        "✨ Специальные анимации")),
	                    st::boxLabel),
	                st::boxRowPadding);
	        } else {
	            box->addRow(
	                object_ptr<Ui::FlatLabel>(
	                    box,
	                    rpl::single(QString::fromUtf8(
	                        "Подпишись на @uzbekgram_client\n"
	                        "чтобы разблокировать PRO функции!\n\n"
	                        "После подписки нажми \"Проверить\"")),
	                    st::boxLabel),
	                st::boxRowPadding);
	
	            // Subscribe button
	            auto subButton = object_ptr<Ui::GradientButton>(box, QGradientStops{
	                { 0.0, QColor(255, 215, 0) },
	                { 1.0, QColor(255, 140, 0) },
	            });
	            subButton->resize(st::boxWideWidth - 40, 44);
	            subButton->setClickedCallback([=, controller = controller] {
	                openRequiredChannel(controller);
	            });
	
	            const auto subText = Ui::CreateChild<Ui::FlatLabel>(
	                subButton.data(),
	                rpl::single(QString::fromUtf8("ПОДПИСАТЬСЯ")),
	                st::defaultFlatLabel);
	            subText->setAttribute(Qt::WA_TransparentForMouseEvents);
	            subText->setStyleSheet("color: white; font-weight: bold;");
	            			subButton->sizeValue().start([=](QSize size) {
	            				subText->move(
	            					(size.width() - subText->width()) / 2,
	            					(size.height() - subText->height()) / 2);
	            			}, [](const auto &) {}, [] {}, subText->lifetime());	
	            box->addRow(std::move(subButton), st::boxRowPadding + QMargins(0, 20, 0, 0));
	
	            // Check button
	            box->addButton(rpl::single(QString::fromUtf8("Проверить")), [=, controller = controller] {
	                refreshStatus(&controller->session());
	                box->closeBox();
	                showUnlockPopup(controller);
	            });
	        }
	
	        box->addButton(tr::lng_close(), [=] {
	            box->closeBox();
	        });
	    });
	
	    controller->show(std::move(box));
	}
} // namespace Ayu
