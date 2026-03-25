#include "ayu/features/halal_fm/halal_fm.h"

#include "ayu/ayu_settings.h"
#include "boxes/abstract_box.h"
#include "crl/crl.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/labels.h"
#include "ui/rp_widget.h"
#include "window/window_session_controller.h"
#include "rpl/producer.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtWidgets/QApplication>
#include <QtGui/QPainter>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include <gsl/gsl_util>
#include <unzip.h>

namespace Ayu::HalalFm {
namespace {

[[nodiscard]] QString CacheDirPath() {
	const auto base = QStandardPaths::writableLocation(
		QStandardPaths::CacheLocation);
	if (!base.isEmpty()) {
		return QDir(base).filePath("halal_fm");
	}
	return QDir::temp().filePath("halal_fm");
}

[[nodiscard]] QString TimestampPath(const QString &cacheDir) {
	return QDir(cacheDir).filePath("timestamp.txt");
}

[[nodiscard]] bool IsSafeRelativePath(const QString &entry) {
	if (entry.isEmpty()) {
		return false;
	}
	if (entry.startsWith('/') || entry.startsWith('\\')) {
		return false;
	}
	if (entry.contains("..")) {
		return false;
	}
	static const auto kDrive = QRegularExpression("^[a-zA-Z]:");
	return !kDrive.match(entry).hasMatch();
}

[[nodiscard]] bool ExtractZip(
		const QString &zipPath,
		const QString &targetDir,
		QString *error) {
	auto *zip = unzOpen64(zipPath.toUtf8().constData());
	if (!zip) {
		if (error) {
			*error = "Failed to open zip archive.";
		}
		return false;
	}
	const auto closeZip = gsl::finally([&] { unzClose(zip); });

	auto result = unzGoToFirstFile(zip);
	while (result == UNZ_OK) {
		unz_file_info64 info = {};
		if (unzGetCurrentFileInfo64(zip, &info, nullptr, 0, nullptr, 0, nullptr, 0)
				!= UNZ_OK) {
			if (error) {
				*error = "Failed to read file info from zip.";
			}
			return false;
		}

		auto nameBytes = QByteArray(int(info.size_filename) + 1, '\0');
		if (unzGetCurrentFileInfo64(
					zip,
					&info,
					nameBytes.data(),
					uLong(nameBytes.size()),
					nullptr,
					0,
					nullptr,
					0)
				!= UNZ_OK) {
			if (error) {
				*error = "Failed to read file name from zip.";
			}
			return false;
		}

		const auto entry = QString::fromUtf8(nameBytes.constData());
		if (!IsSafeRelativePath(entry)) {
			result = unzGoToNextFile(zip);
			continue;
		}

		const auto destination = QDir(targetDir).filePath(entry);
		if (entry.endsWith('/')) {
			if (!QDir().mkpath(destination)) {
				if (error) {
					*error = "Failed to create directory while extracting zip.";
				}
				return false;
			}
			result = unzGoToNextFile(zip);
			continue;
		}

		if (!QDir().mkpath(QFileInfo(destination).path())) {
			if (error) {
				*error = "Failed to create destination folder for extracted file.";
			}
			return false;
		}

		if (unzOpenCurrentFile(zip) != UNZ_OK) {
			if (error) {
				*error = "Failed to open current file in zip.";
			}
			return false;
		}
		const auto closeCurrent = gsl::finally([&] { unzCloseCurrentFile(zip); });

		QFile out(destination);
		if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			if (error) {
				*error = "Failed to open output file while extracting zip.";
			}
			return false;
		}

		std::array<char, 16 * 1024> buffer = {};
		while (true) {
			const auto read = unzReadCurrentFile(
				zip,
				buffer.data(),
				unsigned(buffer.size()));
			if (read < 0) {
				if (error) {
					*error = "Failed to read compressed file contents.";
				}
				return false;
			}
			if (read == 0) {
				break;
			}
			if (out.write(buffer.data(), read) != read) {
				if (error) {
					*error = "Failed to write extracted file contents.";
				}
				return false;
			}
		}

		result = unzGoToNextFile(zip);
	}

	if (result != UNZ_END_OF_LIST_OF_FILE) {
		if (error) {
			*error = "Zip traversal failed with unexpected status.";
		}
		return false;
	}
	return true;
}

[[nodiscard]] bool DownloadFile(const QString &url, const QString &destination) {
	QNetworkAccessManager manager;
	QNetworkRequest request(QUrl(url));
	auto *reply = manager.get(request);
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();
	const auto cleanup = gsl::finally([&] { reply->deleteLater(); });
	if (reply->error() != QNetworkReply::NoError) {
		return false;
	}
	QFile file(destination);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}
	if (file.write(reply->readAll()) < 0) {
		return false;
	}
	return true;
}

void CollectAudioFiles(const QString &root, std::vector<QString> *result) {
	const auto info = QFileInfo(root);
	if (!info.exists()) {
		return;
	}
	if (info.isFile()) {
		const auto lower = info.fileName().toLower();
		if (lower.endsWith(".mp3")
			|| lower.endsWith(".ogg")
			|| lower.endsWith(".m4a")
			|| lower.endsWith(".wav")) {
			result->push_back(info.absoluteFilePath());
		}
		return;
	}
	QDirIterator it(
		root,
		QDir::Files | QDir::NoDotAndDotDot,
		QDirIterator::Subdirectories);
	while (it.hasNext()) {
		const auto path = it.next();
		const auto lower = QFileInfo(path).fileName().toLower();
		if (lower.endsWith(".mp3")
			|| lower.endsWith(".ogg")
			|| lower.endsWith(".m4a")
			|| lower.endsWith(".wav")) {
			result->push_back(path);
		}
	}
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

class Overlay final : public Ui::RpWidget {
public:
	explicit Overlay(QWidget *parent)
	: Ui::RpWidget(parent) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setAttribute(Qt::WA_TranslucentBackground);
		setFixedHeight(52);
		_timer.setInterval(30);
		QObject::connect(&_timer, &QTimer::timeout, this, [=] {
			update();
		});
		_timer.start();
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		p.setRenderHint(QPainter::Antialiasing);
		p.fillRect(rect(), QColor(0, 0, 0, 120));

		const auto now = QDateTime::currentMSecsSinceEpoch();
		const auto pulse = 1.0 + 0.2 * std::sin((now % 1000) / 1000.0 * 2.0 * M_PI);
		const auto color = ((now / 500) % 2 == 0)
			? QColor(0, 255, 0)
			: QColor(255, 255, 0);

		QFont font = this->font();
		font.setBold(true);
		font.setPointSize(14);
		p.setFont(font);
		p.setPen(color);

		const auto text = QString::fromUtf8("ИГРАЕТ HALAL FM!!!");
		const auto metrics = QFontMetrics(font);
		const auto textWidth = metrics.horizontalAdvance(text);
		const auto textHeight = metrics.height();

		p.save();
		p.translate(width() / 2.0, height() / 2.0);
		p.scale(pulse, pulse);
		p.drawText(
			QRect(
				-textWidth / 2,
				-textHeight / 2,
				textWidth,
				textHeight),
			Qt::AlignCenter,
			text);
		p.restore();
	}

private:
	QTimer _timer;
};

class Manager final : public QObject {
public:
	static Manager &Instance() {
		static auto instance = Manager();
		return instance;
	}

	bool enabled() const {
		return AyuSettings::getInstance().halalFmEnabled;
	}

	QString label() const {
		return enabled()
			? QString::fromUtf8("ВЫРУБИТЬ HALAL FM ❌")
			: QString::fromUtf8("HALAL FM NEW ✅");
	}

	void toggle(Window::SessionController *controller) {
		if (enabled()) {
			stop();
			AyuSettings::set_halalFmEnabled(false);
			AyuSettings::save();
			return;
		}
		start(controller);
	}

	void ensureStartupPopup(Window::SessionController *controller) {
		if (!controller) {
			return;
		}
		if (AyuSettings::getInstance().halalFmPopupShown) {
			return;
		}
		AyuSettings::set_halalFmPopupShown(true);
		AyuSettings::save();

		controller->show(Box([=](not_null<Ui::GenericBox*> box) {
			box->setTitle(rpl::single(QString::fromUtf8("HALAL FM ТЕПЕРЬ ДОСТУПЕН В УЗБЕКГРАМЕ!")));
			box->addRow(object_ptr<Ui::FlatLabel>(
				box,
				rpl::single(QString::fromUtf8("врубите эту имбу")),
				st::boxLabel));
			box->addButton(rpl::single(QString::fromUtf8("врубить ✔")), [=] {
				toggle(controller);
				box->closeBox();
			});
			box->addButton(rpl::single(QString::fromUtf8("потом")), [=] {
				box->closeBox();
			});
		}));
	}

	void ensureOverlay(Window::SessionController *controller) {
		if (!enabled() || !controller) {
			return;
		}
		const auto host = controller->widget().get();
		if (!host) {
			return;
		}
		if (_overlayHost == host && _overlay) {
			_overlay->show();
			return;
		}
		removeOverlay();
		_overlayHost = host;
		host->installEventFilter(this);
		_overlay = std::make_unique<Overlay>(host);
		resizeOverlay();
		_overlay->show();
		_overlay->raise();
	}

	void removeOverlay() {
		if (_overlayHost) {
			_overlayHost->removeEventFilter(this);
		}
		_overlayHost = nullptr;
		_overlay = nullptr;
	}

protected:
	bool eventFilter(QObject *watched, QEvent *event) override {
		if (_overlayHost
			&& watched == _overlayHost
			&& event->type() == QEvent::Resize) {
			resizeOverlay();
		}
		return QObject::eventFilter(watched, event);
	}

private:
	void resizeOverlay() {
		if (_overlay && _overlayHost) {
			_overlay->setGeometry(0, 0, _overlayHost->width(), _overlay->height());
		}
	}

	void start(Window::SessionController *controller) {
		stop();
		_stop = false;
		AyuSettings::set_halalFmEnabled(true);
		AyuSettings::save();
		ensureOverlay(controller);
		_thread = std::thread([this] {
			runLoop();
		});
	}

	void stop() {
		_stop = true;
		if (_thread.joinable()) {
			_thread.join();
		}
		crl::on_main(qApp, [=] {
			removeOverlay();
		});
	}

	void runLoop() {
		const auto cacheDir = CacheDirPath();
		const auto timestampPath = TimestampPath(cacheDir);
		QDir().mkpath(cacheDir);

		auto needDownload = true;
		QFile timestamp(timestampPath);
		if (timestamp.exists() && timestamp.open(QIODevice::ReadOnly)) {
			const auto data = timestamp.readAll().trimmed();
			bool ok = false;
			const auto millis = data.toLongLong(&ok);
			if (ok) {
				constexpr auto k3DaysMs = qint64(3) * 24 * 60 * 60 * 1000;
				needDownload = (QDateTime::currentMSecsSinceEpoch() - millis) > k3DaysMs;
			}
		}

		if (needDownload) {
			QDir(cacheDir).removeRecursively();
			QDir().mkpath(cacheDir);
			const auto zipPath = QDir(cacheDir).filePath("fm.zip");
			if (DownloadFile(
					"https://f003.backblazeb2.com/file/cdn-lutit/halal-fm/fm.zip",
					zipPath)) {
				QString error;
				if (ExtractZip(zipPath, cacheDir, &error)) {
					QFile::remove(zipPath);
					QFile out(timestampPath);
					if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
						out.write(QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
					}
				}
			}
		}

		auto files = std::vector<QString>();
		CollectAudioFiles(cacheDir, &files);
		if (files.empty()) {
			crl::on_main(qApp, [=] {
				AyuSettings::set_halalFmEnabled(false);
				AyuSettings::save();
				removeOverlay();
			});
			return;
		}

		auto rng = std::mt19937(std::random_device{}());
		std::shuffle(files.begin(), files.end(), rng);

		const auto player = PickPlayer();
		if (player.isEmpty()) {
			return;
		}

		auto index = size_t(0);
		while (!_stop) {
			if (index >= files.size()) {
				index = 0;
			}
			QProcess process;
			process.setProcessChannelMode(QProcess::MergedChannels);
			process.start(player, PlayerArguments(player, files[index]));
			if (!process.waitForStarted(3000)) {
				++index;
				continue;
			}
			while (!_stop && process.state() != QProcess::NotRunning) {
				process.waitForFinished(200);
			}
			if (_stop && process.state() != QProcess::NotRunning) {
				process.kill();
				process.waitForFinished(1000);
			}
			++index;
		}
	}

	std::atomic_bool _stop = false;
	std::thread _thread;
	QPointer<QWidget> _overlayHost;
	std::unique_ptr<Overlay> _overlay;
};

} // namespace

bool Enabled() {
	return Manager::Instance().enabled();
}

QString Label() {
	return Manager::Instance().label();
}

void Toggle(Window::SessionController *controller) {
	Manager::Instance().toggle(controller);
}

void EnsureStartupPopup(Window::SessionController *controller) {
	Manager::Instance().ensureStartupPopup(controller);
}

void EnsureOverlay(Window::SessionController *controller) {
	Manager::Instance().ensureOverlay(controller);
}

void RemoveOverlay() {
	Manager::Instance().removeOverlay();
}

} // namespace Ayu::HalalFm
