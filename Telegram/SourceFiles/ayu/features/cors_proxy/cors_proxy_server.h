// This is the source code of UzbekGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace Ayu {

class CorsProxyServer final : public QObject {
	Q_OBJECT

public:
	static CorsProxyServer &instance();

	void start();
	void stop();

	[[nodiscard]] bool isRunning() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] quint16 port() const;

private:
	CorsProxyServer();
	~CorsProxyServer();

	void handleNewConnection();
	void handleClientData();
	void handleClientDisconnected();

	void processRequest(QTcpSocket *client, const QByteArray &requestData);
	void sendOptionsResponse(QTcpSocket *client);
	void proxyRequest(QTcpSocket *client, const QString &method, const QString &targetUrl, const QByteArray &headers, const QByteArray &body);

	void handleProxyResponse();
	void handleProxyFinished();
	void handleProxyError(QNetworkReply::NetworkError error);

	void sendResponse(QTcpSocket *client, int statusCode, const QString &statusText, const QByteArray &headers, const QByteArray &body);
	void sendErrorResponse(QTcpSocket *client, int statusCode, const QString &message);

	QTcpServer *_server = nullptr;
	QNetworkAccessManager *_networkManager = nullptr;

	struct ClientRequest {
		QByteArray buffer;
		bool headersParsed = false;
		int contentLength = 0;
	};
	QHash<QTcpSocket*, ClientRequest> _clientRequests;
	QHash<QNetworkReply*, QTcpSocket*> _pendingReplies;
	QHash<QTcpSocket*, bool> _headersSent;

	quint16 _port = 0;
};

} // namespace Ayu