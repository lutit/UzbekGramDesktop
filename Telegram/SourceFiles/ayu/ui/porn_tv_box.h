#pragma once

#include "ui/layers/box_content.h"
#include "webview/webview_embed.h"

namespace Ayu::Ui {

class PornTvBox : public ::Ui::BoxContent {
public:
	PornTvBox(QWidget*, const QString &url);

protected:
	void prepare() override;

private:
	QString _url;
	std::unique_ptr<Webview::Window> _webview;
};

} // namespace Ayu::Ui
