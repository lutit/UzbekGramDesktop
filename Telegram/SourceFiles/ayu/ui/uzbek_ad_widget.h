#pragma once

#include <QtWidgets/QWidget>

class QLabel;
class QPushButton;
class QTimer;
class QNetworkAccessManager;

namespace Ayu::Ui {

class UzbekAdWidget final : public QWidget {
public:
	struct AdData {
		QString imageUrl;
		QString title;
		QString text;
		QString button;
		QString targetUrl;
	};

	enum class Type {
		Chat = 0,
		Dialogs = 1,
		Startup = 2,
	};

	UzbekAdWidget(Type type, QWidget *parent = nullptr);

private:
	void setupUi();
	void setupStyle();
	void setupAnimation();
	void loadImage();
	void applyAnimatedPalette(bool alt);

	Type _type;
	AdData _ad;
	QWidget *_container = nullptr;
	QLabel *_image = nullptr;
	QLabel *_title = nullptr;
	QLabel *_text = nullptr;
	QPushButton *_action = nullptr;
	QPushButton *_close = nullptr;
	QTimer *_animTimer = nullptr;
	QNetworkAccessManager *_network = nullptr;
	bool _altPalette = false;
};

} // namespace Ayu::Ui
