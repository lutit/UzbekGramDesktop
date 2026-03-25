#include "ayu/features/allah_call/allah_call.h"

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

namespace Ayu::AllahCall {
namespace {

[[nodiscard]] QString CacheDirPath() {
	const auto base = QStandardPaths::writableLocation(
		QStandardPaths::CacheLocation);
	if (!base.isEmpty()) {
		return QDir(base).filePath("allah_call");
	}
	return QDir::temp().filePath("allah_call");
}

[[nodiscard]] QString PickPlayer() {
	for (const auto &name : {
			QString("ffplay"),
			QString("mpv"),
			QString("vlc") }) {
		const auto path = QStandardPaths::findExecutable(name);
		if (!path.isEmpty()) {
			return path;
		}
	}
	return QString();
}

[[nodiscard]] QStringList PlayerArguments(
		const QString &playerPath,
		const QString &audioPath) {
	const auto exe = QFileInfo(playerPath).baseName().toLower();
	if (exe == "ffplay") {
		return {
			"-nodisp",
			"-autoexit",
			"-loglevel",
			"quiet",
			audioPath,
		};
	} else if (exe == "mpv") {
		return {
			"--no-video",
			"--really-quiet",
			"--force-window=no",
			"--audio-display=no",
			audioPath,
		};
	}
	return {
		"--intf",
		"dummy",
		"--play-and-exit",
		audioPath,
	};
}

[[nodiscard]] bool DownloadFile(const QString &url, const QString &destination) {
	QNetworkAccessManager manager;
	QNetworkRequest request(QUrl(url));
	auto *reply = manager.get(request);
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();
	reply->deleteLater();
	if (reply->error() != QNetworkReply::NoError) {
		return false;
	}
	QFile file(destination);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}
	return file.write(reply->readAll()) >= 0;
}

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
		startAudioLoop();
	}

	~Window() override {
		stopAudioLoop();
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

	void startAudioLoop() {
		_stop = false;
		_audioThread = std::thread([this] {
			const auto urls = std::vector<QString>{
				"https://f003.backblazeb2.com/file/cdn-lutit/r/allah1.ogg",
				"https://f003.backblazeb2.com/file/cdn-lutit/r/allah2.ogg",
				"https://f003.backblazeb2.com/file/cdn-lutit/r/%D0%A3%D0%B7%D0%B1%D0%B5%D0%BA%D0%93%D1%80%D0%B0%D0%BC.mp3",
				"https://f003.backblazeb2.com/file/cdn-lutit/r/Flappy+bird+September+edition+Nasheed.mp3",
			};
			const auto cacheDir = CacheDirPath();
			QDir().mkpath(cacheDir);

			auto files = std::vector<QString>();
			for (const auto &url : urls) {
				if (_stop) {
					return;
				}
				auto fileName = QFileInfo(QUrl(url).path()).fileName();
				if (fileName.isEmpty()) {
					fileName = QString::number(qHash(url));
				}
				const auto path = QDir(cacheDir).filePath(fileName);
				if (!QFileInfo::exists(path)) {
					DownloadFile(url, path);
				}
				if (QFileInfo::exists(path)) {
					files.push_back(path);
				}
			}

			if (files.empty() || _stop) {
				return;
			}
			const auto player = PickPlayer();
			if (player.isEmpty()) {
				return;
			}

			auto first = files.front();
			for (const auto &path : files) {
				if (QFileInfo(path).fileName() == "allah1.ogg") {
					first = path;
					break;
				}
			}
			playAudioSync(first, player);

			auto rng = std::mt19937(std::random_device{}());
			while (!_stop) {
				for (auto i = 0; i != 20 && !_stop; ++i) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				if (_stop) {
					break;
				}
				const auto index = std::uniform_int_distribution<int>(
					0,
					int(files.size()) - 1)(rng);
				playAudioSync(files[index], player);
			}
		});
	}

	void stopAudioLoop() {
		_stop = true;
		if (_audioThread.joinable()) {
			_audioThread.join();
		}
	}

	void playAudioSync(const QString &filePath, const QString &playerPath) {
		if (_stop) {
			return;
		}
		QProcess process;
		process.setProcessChannelMode(QProcess::MergedChannels);
		process.start(playerPath, PlayerArguments(playerPath, filePath));
		if (!process.waitForStarted(3000)) {
			return;
		}
		while (!_stop && process.state() != QProcess::NotRunning) {
			process.waitForFinished(200);
		}
		if (_stop && process.state() != QProcess::NotRunning) {
			process.kill();
			process.waitForFinished(1000);
		}
	}

	QElapsedTimer _elapsed;
	QPointer<QTimer> _tick;
	QPointer<QLabel> _timer;
	std::atomic_bool _stop = false;
	std::thread _audioThread;
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
