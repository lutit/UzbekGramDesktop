// This is the source code of UzbekGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
#include "cors_proxy_server.h"

#include <QtCore/QUrl>
#include <QtCore/QRegularExpression>
#include <QtCore/QDebug>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QHostAddress>
#include <iostream>

#define CORS_LOG(msg) std::cerr << "[CorsProxy] " << msg << std::endl

namespace Ayu {

CorsProxyServer &CorsProxyServer::instance() {
	static CorsProxyServer instance;
	return instance;
}

CorsProxyServer::CorsProxyServer()
: QObject(nullptr)
, _server(new QTcpServer(this))
, _networkManager(new QNetworkAccessManager(this)) {
	connect(_server, &QTcpServer::newConnection, this, &CorsProxyServer::handleNewConnection);
}

CorsProxyServer::~CorsProxyServer() {
	stop();
}

void CorsProxyServer::start() {
	if (_server->isListening()) {
		return;
	}

	// Listen on localhost with port 0 (system will assign a free port)
	if (_server->listen(QHostAddress::LocalHost, 0)) {
		_port = _server->serverPort();
		CORS_LOG("Started on 127.0.0.1:" << _port);
	} else {
		CORS_LOG("Failed to start: " << _server->errorString().toStdString());
	}
}

void CorsProxyServer::stop() {
	if (_server->isListening()) {
		_server->close();
		CORS_LOG("Stopped");
	}

	// Close all pending replies
	for (auto reply : _pendingReplies.keys()) {
		reply->abort();
		reply->deleteLater();
	}
	_pendingReplies.clear();

	// Close all client connections
	for (auto client : _clientRequests.keys()) {
		client->close();
		client->deleteLater();
	}
	_clientRequests.clear();
	_headersSent.clear();

	_port = 0;
}

bool CorsProxyServer::isRunning() const {
	return _server->isListening();
}

QString CorsProxyServer::address() const {
	if (!isRunning()) {
		return QString();
	}
	return QString("http://127.0.0.1:%1").arg(_port);
}

quint16 CorsProxyServer::port() const {
	return _port;
}

void CorsProxyServer::handleNewConnection() {
	while (_server->hasPendingConnections()) {
		auto client = _server->nextPendingConnection();
		_clientRequests[client] = ClientRequest();

		CORS_LOG("New connection from " << client->peerAddress().toString().toStdString() << ":" << client->peerPort());

		connect(client, &QTcpSocket::readyRead, this, &CorsProxyServer::handleClientData);
		connect(client, &QTcpSocket::disconnected, this, &CorsProxyServer::handleClientDisconnected);
	}
}

void CorsProxyServer::handleClientData() {
	auto client = qobject_cast<QTcpSocket*>(sender());
	if (!client || !_clientRequests.contains(client)) {
		return;
	}

	auto &request = _clientRequests[client];
	request.buffer.append(client->readAll());

	// Check if we have complete headers
	if (!request.headersParsed) {
		const auto headerEnd = request.buffer.indexOf("\r\n\r\n");
		if (headerEnd == -1) {
			return; // Wait for more data
		}

		request.headersParsed = true;

		// Check Content-Length
		static const QRegularExpression contentLengthRe(
			"Content-Length:\\s*(\\d+)",
			QRegularExpression::CaseInsensitiveOption);
		const auto match = contentLengthRe.match(QString::fromUtf8(request.buffer));
		if (match.hasMatch()) {
			request.contentLength = match.captured(1).toInt();
		}
	}

	// Check if we have the complete body
	const auto headerEnd = request.buffer.indexOf("\r\n\r\n");
	const auto bodyStart = headerEnd + 4;
	const auto bodyLength = request.buffer.size() - bodyStart;

	if (bodyLength >= request.contentLength) {
		processRequest(client, request.buffer);
		_clientRequests.remove(client);
	}
}

void CorsProxyServer::handleClientDisconnected() {
	auto client = qobject_cast<QTcpSocket*>(sender());
	if (!client) {
		return;
	}

	_clientRequests.remove(client);
	_headersSent.remove(client);

	// Cancel any pending replies for this client
	for (auto it = _pendingReplies.begin(); it != _pendingReplies.end();) {
		if (it.value() == client) {
			it.key()->abort();
			it.key()->deleteLater();
			it = _pendingReplies.erase(it);
		} else {
			++it;
		}
	}

	client->deleteLater();
}

void CorsProxyServer::processRequest(QTcpSocket *client, const QByteArray &requestData) {
	CORS_LOG("Processing request, data size: " << requestData.size());

	// Parse the request line
	const auto headerEnd = requestData.indexOf("\r\n\r\n");
	const auto headersSection = requestData.left(headerEnd);
	const auto body = requestData.mid(headerEnd + 4);

	const auto firstLineEnd = headersSection.indexOf("\r\n");
	const auto requestLine = QString::fromUtf8(headersSection.left(firstLineEnd));
	const auto headers = headersSection.mid(firstLineEnd + 2);

	CORS_LOG("Request line: " << requestLine.toStdString());

	// Parse: METHOD /URL HTTP/1.1
	static const QRegularExpression requestLineRe("^(\\w+)\\s+(/\\S*)\\s+HTTP/");
	const auto match = requestLineRe.match(requestLine);
	if (!match.hasMatch()) {
		CORS_LOG("Bad request - failed to parse request line");
		sendErrorResponse(client, 400, "Bad Request");
		return;
	}

	const auto method = match.captured(1);
	auto path = match.captured(2);

	CORS_LOG("Method: " << method.toStdString() << ", Path: " << path.toStdString());

	// Handle OPTIONS preflight
	if (method == "OPTIONS") {
		CORS_LOG("Handling OPTIONS preflight");
		sendOptionsResponse(client);
		return;
	}

	// Extract target URL from path (format: /https://example.com/path or /http://example.com/path)
	if (path.startsWith("/")) {
		path = path.mid(1);
	}

	// URL decode the path
	path = QUrl::fromPercentEncoding(path.toUtf8());

	CORS_LOG("Target URL after decode: " << path.toStdString());

	// Validate URL
	const QUrl targetUrl(path);
	if (!targetUrl.isValid() || (targetUrl.scheme() != "http" && targetUrl.scheme() != "https")) {
		CORS_LOG("Invalid target URL - scheme: " << targetUrl.scheme().toStdString() << ", isValid: " << targetUrl.isValid());
		sendErrorResponse(client, 400, "Invalid target URL");
		return;
	}

	proxyRequest(client, method, path, headers, body);
}

void CorsProxyServer::sendOptionsResponse(QTcpSocket *client) {
	const QByteArray response =
		"HTTP/1.1 204 No Content\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: GET, POST, PUT, DELETE, PATCH, OPTIONS\r\n"
		"Access-Control-Allow-Headers: *\r\n"
		"Access-Control-Max-Age: 86400\r\n"
		"Content-Length: 0\r\n"
		"Connection: close\r\n"
		"\r\n";

	client->write(response);
	client->flush();
	client->disconnectFromHost();
}

void CorsProxyServer::proxyRequest(
		QTcpSocket *client,
		const QString &method,
		const QString &targetUrl,
		const QByteArray &headers,
		const QByteArray &body) {

	CORS_LOG("Proxying " << method.toStdString() << " request to: " << targetUrl.toStdString());

	auto request = QNetworkRequest(QUrl(targetUrl));

	// Parse and forward headers (except Host which QNetworkRequest sets automatically)
	const auto headerLines = headers.split('\n');
	for (const auto &line : headerLines) {
		const auto trimmed = line.trimmed();
		if (trimmed.isEmpty()) continue;

		const auto colonPos = trimmed.indexOf(':');
		if (colonPos == -1) continue;

		const auto name = trimmed.left(colonPos).trimmed();
		const auto value = trimmed.mid(colonPos + 1).trimmed();

		// Skip headers that should not be forwarded
		const auto lowerName = name.toLower();
		if (lowerName == "host" ||
			lowerName == "connection" ||
			lowerName == "accept-encoding" ||
			lowerName == "origin" ||
			lowerName == "referer") {
			continue;
		}

		request.setRawHeader(name, value);
	}

	// Set a reasonable User-Agent if not provided
	if (!request.hasRawHeader("User-Agent")) {
		request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
	}

	QNetworkReply *reply = nullptr;

	if (method == "GET") {
		reply = _networkManager->get(request);
	} else if (method == "POST") {
		reply = _networkManager->post(request, body);
	} else if (method == "PUT") {
		reply = _networkManager->put(request, body);
	} else if (method == "DELETE") {
		reply = _networkManager->deleteResource(request);
	} else if (method == "HEAD") {
		reply = _networkManager->head(request);
	} else {
		// Custom method
		reply = _networkManager->sendCustomRequest(request, method.toUtf8(), body);
	}

	if (!reply) {
		sendErrorResponse(client, 500, "Failed to create request");
		return;
	}

	_pendingReplies[reply] = client;
	_headersSent[client] = false;

	connect(reply, &QNetworkReply::readyRead, this, &CorsProxyServer::handleProxyResponse);
	connect(reply, &QNetworkReply::finished, this, &CorsProxyServer::handleProxyFinished);
	connect(reply, &QNetworkReply::errorOccurred, this, &CorsProxyServer::handleProxyError);
}

void CorsProxyServer::handleProxyResponse() {
	auto reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply || !_pendingReplies.contains(reply)) {
		return;
	}

	auto client = _pendingReplies[reply];
	if (!client || client->state() != QAbstractSocket::ConnectedState) {
		return;
	}

	// Send headers first time
	if (!_headersSent.value(client, false)) {
		_headersSent[client] = true;

		const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const auto statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

		CORS_LOG("Response received - status: " << statusCode << " " << statusText.toStdString() << ", URL: " << reply->url().toString().toStdString());

		QByteArray responseHeaders;
		responseHeaders.append("HTTP/1.1 ");
		responseHeaders.append(QByteArray::number(statusCode));
		responseHeaders.append(" ");
		responseHeaders.append(statusText.toUtf8());
		responseHeaders.append("\r\n");

		// Add CORS headers
		responseHeaders.append("Access-Control-Allow-Origin: *\r\n");
		responseHeaders.append("Access-Control-Expose-Headers: *\r\n");

		// Forward response headers
		const auto rawHeaders = reply->rawHeaderPairs();
		for (const auto &header : rawHeaders) {
			const auto lowerName = header.first.toLower();
			// Skip headers that we handle ourselves or should not forward
			// Note: content-encoding is skipped because QNetworkAccessManager 
			// automatically decompresses gzip/deflate, so we send uncompressed data
			if (lowerName == "access-control-allow-origin" ||
				lowerName == "access-control-expose-headers" ||
				lowerName == "transfer-encoding" ||
				lowerName == "connection" ||
				lowerName == "content-encoding" ||
				lowerName == "content-length") {
				continue;
			}
			responseHeaders.append(header.first);
			responseHeaders.append(": ");
			responseHeaders.append(header.second);
			responseHeaders.append("\r\n");
		}

		// We'll use chunked transfer encoding for streaming
		responseHeaders.append("Transfer-Encoding: chunked\r\n");
		responseHeaders.append("Connection: close\r\n");
		responseHeaders.append("\r\n");

		client->write(responseHeaders);
	}

	// Stream the data using chunked encoding
	const auto data = reply->readAll();
	if (!data.isEmpty()) {
		// Write chunk size in hex
		client->write(QByteArray::number(data.size(), 16));
		client->write("\r\n");
		client->write(data);
		client->write("\r\n");
		client->flush();
	}
}

void CorsProxyServer::handleProxyFinished() {
	auto reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) {
		return;
	}

	auto client = _pendingReplies.take(reply);

	if (client && client->state() == QAbstractSocket::ConnectedState) {
		// Handle case where no data was ever sent (e.g., redirects handled internally)
		if (!_headersSent.value(client, false)) {
			const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			const auto statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

			if (statusCode == 0) {
				// Network error occurred
				sendErrorResponse(client, 502, "Bad Gateway: " + reply->errorString());
			} else {
				// Send headers and any remaining data
				handleProxyResponse();
			}
		}

		// Read any remaining data
		const auto remainingData = reply->readAll();
		if (!remainingData.isEmpty()) {
			client->write(QByteArray::number(remainingData.size(), 16));
			client->write("\r\n");
			client->write(remainingData);
			client->write("\r\n");
		}

		// Send final chunk (0-length) to signal end
		if (_headersSent.value(client, false)) {
			client->write("0\r\n\r\n");
		}

		client->flush();
		client->disconnectFromHost();
	}

	_headersSent.remove(client);
	reply->deleteLater();
}

void CorsProxyServer::handleProxyError(QNetworkReply::NetworkError error) {
	auto reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply || !_pendingReplies.contains(reply)) {
		return;
	}

	auto client = _pendingReplies.value(reply);
	if (!client || client->state() != QAbstractSocket::ConnectedState) {
		return;
	}

	// Only send error if we haven't sent headers yet
	if (!_headersSent.value(client, false)) {
		QString errorMessage = reply->errorString();
		CORS_LOG("Request error: " << errorMessage.toStdString());

		int statusCode = 502; // Bad Gateway
		switch (error) {
			case QNetworkReply::ConnectionRefusedError:
			case QNetworkReply::RemoteHostClosedError:
			case QNetworkReply::HostNotFoundError:
				statusCode = 502;
				break;
			case QNetworkReply::TimeoutError:
				statusCode = 504; // Gateway Timeout
				break;
			case QNetworkReply::ContentNotFoundError:
				statusCode = 404;
				break;
			default:
				statusCode = 502;
				break;
		}

		sendErrorResponse(client, statusCode, errorMessage);
	}
}

void CorsProxyServer::sendResponse(
		QTcpSocket *client,
		int statusCode,
		const QString &statusText,
		const QByteArray &headers,
		const QByteArray &body) {

	QByteArray response;
	response.append("HTTP/1.1 ");
	response.append(QByteArray::number(statusCode));
	response.append(" ");
	response.append(statusText.toUtf8());
	response.append("\r\n");
	response.append("Access-Control-Allow-Origin: *\r\n");
	response.append(headers);
	response.append("Content-Length: ");
	response.append(QByteArray::number(body.size()));
	response.append("\r\n");
	response.append("Connection: close\r\n");
	response.append("\r\n");
	response.append(body);

	client->write(response);
	client->flush();
	client->disconnectFromHost();
}

void CorsProxyServer::sendErrorResponse(QTcpSocket *client, int statusCode, const QString &message) {
	const auto body = QString("{\"error\":\"%1\",\"code\":%2}").arg(message).arg(statusCode).toUtf8();
	sendResponse(client, statusCode, message, "Content-Type: application/json\r\n", body);
}

} // namespace Ayu