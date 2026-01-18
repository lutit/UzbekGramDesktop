// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "ayu/shamala_chat.h"
#include "ayu/ayu_settings.h"
#include "core/application.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Ayu {

namespace {
	QString KeyFromId(FullMsgId id) {
		return QString::number(id.peer.value) + "_" + QString::number(id.msg);
	}
}

ShamalaChat &ShamalaChat::instance() {
	static ShamalaChat inst;
	return inst;
}

ShamalaChat::ShamalaChat() {
	loadStorage();
}

void ShamalaChat::rewrite(
		const QString &text,
		Fn<void(QString)> onSuccess,
		Fn<void()> onFail) {

	const QString urlStr = "https://gptuzbek.ddosxd.ru/v1/chat/completions";
	const QString apiKey = "hui";

	QNetworkRequest request(QUrl(urlStr));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

	QJsonObject systemMessage;
	systemMessage["role"] = "system";
	systemMessage["content"] = "Ты — узбекский гопник. Перепиши сообщение пользователя на русский с жестким узбекским акцентом, используя мат, слова 'э', 'дон', 'жи есть', 'брат'. Смысл сохрани, но сделай максимально тупо и смешно. Ответ должен содержать ТОЛЬКО переписанный текст.";

	QJsonObject userMessage;
	userMessage["role"] = "user";
	userMessage["content"] = text;

	QJsonArray messages;
	messages.append(systemMessage);
	messages.append(userMessage);

	QJsonObject body;
	body["model"] = "gpt-3.5-turbo"; // Or whatever model the endpoint expects
	body["messages"] = messages;
	body["temperature"] = 0.7;

	QNetworkReply *reply = _nam.post(request, QJsonDocument(body).toJson());

	connect(reply, &QNetworkReply::finished, [reply, onSuccess, onFail]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (onFail) onFail();
			return;
		}

		QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
		if (doc.isNull()) {
			if (onFail) onFail();
			return;
		}

		QString result = doc["choices"][0]["message"]["content"].toString().trimmed();
		if (result.isEmpty()) {
			if (onFail) onFail();
			return;
		}

		if (onSuccess) onSuccess(result);
	});
}

void ShamalaChat::saveOriginal(FullMsgId id, const QString &text) {
	_originalTexts[KeyFromId(id)] = text;
	saveStorage();
}

QString ShamalaChat::getOriginal(FullMsgId id) {
	return _originalTexts.value(KeyFromId(id));
}

QString ShamalaChat::getStoragePath() {
	return AyuSettings::getSettingsPath() + "_shamala_history.json";
}

void ShamalaChat::loadStorage() {
	QFile file(getStoragePath());
	if (!file.open(QIODevice::ReadOnly)) return;

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();
	for (auto it = obj.begin(); it != obj.end(); ++it) {
		_originalTexts[it.key()] = it.value().toString();
	}
}

void ShamalaChat::saveStorage() {
	QJsonObject obj;
	for (auto it = _originalTexts.begin(); it != _originalTexts.end(); ++it) {
		obj[it.key()] = it.value();
	}
	QFile file(getStoragePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(obj).toJson());
	}
}

} // namespace Ayu
