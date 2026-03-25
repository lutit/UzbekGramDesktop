#include "ayu/ui/uzbek_ad_widget.h"

#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QPixmap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <vector>

namespace Ayu::Ui {
namespace {

[[nodiscard]] std::vector<UzbekAdWidget::AdData> BuildAds() {
	return {
		{
			.imageUrl = "https://cdn.lutit.xyz/r/c95fcec21d2f4db55ce3f1728971f901.png",
			.title = QString::fromUtf8("НОВЫЙ ПРОЦЕССОР ОТ UZBEK COMPANY"),
			.text = QString::fromUtf8("ТОЛЬКО СЕЙЧАС ПО СКИДКЕ ВСЕГО 1337 СОМ !!!!!"),
			.button = QString::fromUtf8("ПРОДАТЬ ПОЧКУ"),
			.targetUrl = "https://t.me/uzbekgram_client",
		},
		{
			.imageUrl = "https://cdn.lutit.xyz/r/my/linus.png",
			.title = QString::fromUtf8("ВАС ПОСЛАЛ НАХУЙ ТОРВАЛЬДС!"),
			.text = QString::fromUtf8("да"),
			.button = QString::fromUtf8("ПОСЛАТЬ НАХУЙ В ОТВЕТ"),
			.targetUrl = "",
		},
		{
			.imageUrl = "https://cdn.lutit.xyz/r/my/uzbekgram.jpg",
			.title = QString::fromUtf8("УСТАНОВИТЕ ОБНОВЛЕНИЕ УЗБЕКГРАМ"),
			.text = QString::fromUtf8("PORN TV FEATURE NEW DA UZBEKISTAN ALOOOOOOOO"),
			.button = QString::fromUtf8("СКАЧАТЬ МЕССЕНДЖЕР АЛЛАХА"),
			.targetUrl = "https://uzbekgram.lutit.xyz",
		},
		{
			.imageUrl = "https://cdn.lutit.xyz/r/my/sepi.jpg",
			.title = QString::fromUtf8("ВАС ПРИГЛАСИЛИ В СЕКС ЧАТ СЕПИ"),
			.text = QString::fromUtf8("ВЫ НЕ МОЖЕТЕ ОТКАЗАТЬСЯ ❌ ТАК КАК Я УЗБЕК"),
			.button = QString::fromUtf8("принять"),
			.targetUrl = "https://t.me/thebesttexteditor",
		},
		{
			.imageUrl = "https://cdn.lutit.xyz/r/my/photo_2026-03-25_18-09-09.jpg",
			.title = QString::fromUtf8("АЛЬКАФОН БИЗНЕС"),
			.text = QString::fromUtf8("ОТКРОЙТЕ ВОЗМОЖНОСТИ СВОЕГО ПОТОЛКА НА ВСЕ 1000000000% ЗА 3000 СОМ"),
			.button = QString::fromUtf8("ЗАПУСТИТЬ РАКЕТУ"),
			.targetUrl = "https://t.me/alkafon_bizness",
		},
	};
}

[[nodiscard]] UzbekAdWidget::AdData PickRandomAd() {
	const auto ads = BuildAds();
	if (ads.empty()) {
		return {};
	}
	const auto index = QRandomGenerator::global()->bounded(int(ads.size()));
	return ads[std::max(0, index)];
}

} // namespace

UzbekAdWidget::UzbekAdWidget(Type type, QWidget *parent)
: QWidget(parent)
, _type(type)
, _ad(PickRandomAd()) {
	setAttribute(Qt::WA_StyledBackground, true);
	setupUi();
	setupStyle();
	setupAnimation();
	loadImage();
}

void UzbekAdWidget::setupUi() {
	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	if (_type == Type::Startup) {
		setStyleSheet("background: rgba(0,0,0,136);");
	}

	_container = new QWidget(this);
	_container->setObjectName("adFrame");
	root->addWidget(_container, 0, (_type == Type::Startup) ? Qt::AlignCenter : Qt::AlignTop);

	auto *layout = new QVBoxLayout(_container);
	layout->setContentsMargins(
		(_type == Type::Startup) ? 20 : ((_type == Type::Dialogs) ? 4 : 10),
		(_type == Type::Startup) ? 20 : ((_type == Type::Dialogs) ? 4 : 10),
		(_type == Type::Startup) ? 20 : ((_type == Type::Dialogs) ? 4 : 10),
		(_type == Type::Startup) ? 20 : ((_type == Type::Dialogs) ? 4 : 10));
	layout->setSpacing(8);

	_image = new QLabel(_container);
	_image->setMinimumHeight((_type == Type::Startup) ? 250 : 150);
	_image->setAlignment(Qt::AlignCenter);
	_image->setScaledContents(true);
	layout->addWidget(_image);

	_title = new QLabel(_ad.title, _container);
	_title->setWordWrap(true);
	_title->setAlignment(Qt::AlignLeft);
	_title->setTextInteractionFlags(Qt::NoTextInteraction);
	layout->addWidget(_title);

	_text = new QLabel(_ad.text, _container);
	_text->setWordWrap(true);
	_text->setAlignment(Qt::AlignLeft);
	_text->setTextInteractionFlags(Qt::NoTextInteraction);
	layout->addWidget(_text);

	_action = new QPushButton(_ad.button, _container);
	QObject::connect(_action, &QPushButton::clicked, this, [=] {
		if (!_ad.targetUrl.isEmpty()) {
			QDesktopServices::openUrl(QUrl(_ad.targetUrl));
		}
	});
	layout->addWidget(_action);

	if (_type == Type::Startup) {
		_close = new QPushButton(QString::fromUtf8("ЗАКРЫТЬ ЭТО ДЕРЬМО"), _container);
		QObject::connect(_close, &QPushButton::clicked, this, [=] {
			hide();
			deleteLater();
		});
		layout->addWidget(_close);
	}

	auto *shadow = new QGraphicsDropShadowEffect(_container);
	shadow->setBlurRadius(24);
	shadow->setOffset(0, 8);
	shadow->setColor(QColor(0, 0, 0, 140));
	_container->setGraphicsEffect(shadow);
}

void UzbekAdWidget::setupStyle() {
	if (_type == Type::Chat) {
		_container->setStyleSheet("background: rgba(0,0,0,68); border-radius: 10px;");
	} else {
		_container->setStyleSheet("background: #ff0000; border-radius: 10px;");
	}

	_title->setStyleSheet(QString::fromLatin1(
		"font-weight: 700; color: #ffff00; font-size: %1px;")
		.arg((_type == Type::Startup) ? 20 : 16));
	_text->setStyleSheet(QString::fromLatin1(
		"font-weight: 500; color: #ffffff; font-size: %1px;")
		.arg((_type == Type::Startup) ? 18 : 14));
	_action->setStyleSheet(QString::fromLatin1(
		"background: #0000ff; color: #ffffff; font-weight: 700; border: 0; border-radius: 8px; padding: %1px;")
		.arg((_type == Type::Startup) ? 15 : 10));
	if (_close) {
		_close->setStyleSheet(
			"background: #444444; color: #d3d3d3; font-weight: 700; border: 0; border-radius: 8px; padding: 15px;");
	}
}

void UzbekAdWidget::setupAnimation() {
	_animTimer = new QTimer(this);
	_animTimer->setInterval(300);
	QObject::connect(_animTimer, &QTimer::timeout, this, [=] {
		_altPalette = !_altPalette;
		applyAnimatedPalette(_altPalette);
		const auto pulse = _altPalette ? 6 : 0;
		const auto margin = (_type == Type::Dialogs) ? 4 : 10;
		if (auto *layout = qobject_cast<QVBoxLayout*>(_container->layout())) {
			layout->setContentsMargins(margin + pulse, margin, margin + pulse, margin);
		}
	});
	_animTimer->start();
}

void UzbekAdWidget::loadImage() {
	if (_ad.imageUrl.isEmpty()) {
		return;
	}
	_network = new QNetworkAccessManager(this);
	auto *reply = _network->get(QNetworkRequest(QUrl(_ad.imageUrl)));
	QObject::connect(reply, &QNetworkReply::finished, this, [=] {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			return;
		}
		auto pixmap = QPixmap();
		if (!pixmap.loadFromData(reply->readAll())) {
			return;
		}
		_image->setPixmap(pixmap.scaled(
			_image->size(),
			Qt::KeepAspectRatioByExpanding,
			Qt::SmoothTransformation));
	});
}

void UzbekAdWidget::applyAnimatedPalette(bool alt) {
	if (alt) {
		_title->setStyleSheet(QString::fromLatin1(
			"font-weight: 700; color: #ff0000; font-size: %1px;")
			.arg((_type == Type::Startup) ? 20 : 16));
		_text->setStyleSheet(QString::fromLatin1(
			"font-weight: 500; color: #ffff00; font-size: %1px;")
			.arg((_type == Type::Startup) ? 18 : 14));
		_action->setStyleSheet(QString::fromLatin1(
			"background: #00ff00; color: #000000; font-weight: 700; border: 0; border-radius: 8px; padding: %1px;")
			.arg((_type == Type::Startup) ? 15 : 10));
	} else {
		_title->setStyleSheet(QString::fromLatin1(
			"font-weight: 700; color: #ffff00; font-size: %1px;")
			.arg((_type == Type::Startup) ? 20 : 16));
		_text->setStyleSheet(QString::fromLatin1(
			"font-weight: 500; color: #ffffff; font-size: %1px;")
			.arg((_type == Type::Startup) ? 18 : 14));
		_action->setStyleSheet(QString::fromLatin1(
			"background: #0000ff; color: #ffffff; font-weight: 700; border: 0; border-radius: 8px; padding: %1px;")
			.arg((_type == Type::Startup) ? 15 : 10));
	}
}

} // namespace Ayu::Ui
