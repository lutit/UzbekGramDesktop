#include "ayu/features/allah_call/allah_call.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace Ayu::AllahCall {
namespace {

class Window final : public QWidget {
public:
	explicit Window(QWidget *parent)
	: QWidget(parent) {
		setAttribute(Qt::WA_DeleteOnClose);
		setWindowTitle(QString::fromUtf8("Аллах"));
		resize(420, 760);

		auto *root = new QVBoxLayout(this);
		root->setContentsMargins(0, 0, 0, 0);
		root->setSpacing(0);

		auto *top = new QWidget(this);
		top->setStyleSheet("background: #0A8AD4;");
		top->setMinimumHeight(250);
		root->addWidget(top);

		auto *topLayout = new QVBoxLayout(top);
		topLayout->setContentsMargins(24, 34, 24, 18);
		topLayout->setSpacing(8);

		auto *title = new QLabel(QString::fromUtf8("Аллах"), top);
		title->setStyleSheet("color: #EAF7FF; font-size: 50px;");
		topLayout->addWidget(title);

		auto *row = new QHBoxLayout();
		row->setContentsMargins(0, 0, 0, 0);
		row->setSpacing(8);
		auto *number = new QLabel(QString::fromUtf8("Мобильный 666"), top);
		number->setStyleSheet("color: #D0E5F6; font-size: 17px;");
		row->addWidget(number);
		row->addStretch();
		_timer = new QLabel(top);
		_timer->setStyleSheet("color: #D0E5F6; font-size: 17px;");
		row->addWidget(_timer);
		topLayout->addLayout(row);
		topLayout->addStretch();

		auto *body = new QWidget(this);
		body->setStyleSheet("background: #585858;");
		root->addWidget(body, 1);

		auto *bodyLayout = new QVBoxLayout(body);
		bodyLayout->setContentsMargins(0, 56, 0, 24);
		bodyLayout->setSpacing(0);
		bodyLayout->setAlignment(Qt::AlignHCenter);

		auto *head = new QWidget(body);
		head->setFixedSize(96, 96);
		head->setStyleSheet("background: #737373; border-radius: 48px;");
		bodyLayout->addWidget(head, 0, Qt::AlignHCenter);

		auto *avatarBody = new QWidget(body);
		avatarBody->setFixedSize(190, 70);
		avatarBody->setStyleSheet("background: #737373; border-radius: 20px;");
		bodyLayout->addSpacing(36);
		bodyLayout->addWidget(avatarBody, 0, Qt::AlignHCenter);

		auto *endButton = new QPushButton(QString::fromUtf8("X"), body);
		endButton->setFixedSize(86, 86);
		endButton->setStyleSheet(
			"QPushButton { background: #FF1E4A; color: white; border: 0; "
			"border-radius: 43px; font-size: 28px; font-weight: 700; }");
		bodyLayout->addSpacing(44);
		bodyLayout->addWidget(endButton, 0, Qt::AlignHCenter);
		bodyLayout->addStretch();

		QObject::connect(endButton, &QPushButton::clicked, this, [=] {
			close();
		});

		_elapsed.start();
		_tick = new QTimer(this);
		QObject::connect(_tick, &QTimer::timeout, this, [=] {
			updateTimer();
		});
		_tick->start(1000);
		updateTimer();
	}

private:
	void updateTimer() {
		const auto seconds = (_elapsed.elapsed() / 1000) + 1;
		const auto mm = seconds / 60;
		const auto ss = seconds % 60;
		_timer->setText(QString::fromLatin1("%1:%2")
			.arg(mm, 2, 10, QLatin1Char('0'))
			.arg(ss, 2, 10, QLatin1Char('0')));
	}

	QElapsedTimer _elapsed;
	QPointer<QTimer> _tick;
	QPointer<QLabel> _timer;
};

} // namespace

void Open(QWidget *parent) {
	static QPointer<Window> window;
	if (!window.isNull()) {
		window->show();
		window->raise();
		window->activateWindow();
		return;
	}
	window = new Window(parent);
	window->show();
	window->raise();
	window->activateWindow();
}

} // namespace Ayu::AllahCall
