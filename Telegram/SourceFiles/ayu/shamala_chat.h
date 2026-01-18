// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#pragma once

#include "data/data_msg_id.h"
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

namespace Ayu {

class ShamalaChat : public QObject {
	Q_OBJECT

public:
	static ShamalaChat &instance();

	void rewrite(
		const QString &text,
		Fn<void(QString)> onSuccess,
		Fn<void()> onFail);

	void saveOriginal(FullMsgId id, const QString &text);
	QString getOriginal(FullMsgId id);

private:
	ShamalaChat();
	void loadStorage();
	void saveStorage();
	QString getStoragePath();

	QNetworkAccessManager _nam;
	QMap<QString, QString> _originalTexts; // Key: peer_msg
};

} // namespace Ayu
