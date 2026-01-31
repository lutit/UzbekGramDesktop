#include "ayu/ui/porn_tv_box.h"

#include "ui/widgets/labels.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"

namespace Ayu::Ui {

PornTvBox::PornTvBox(QWidget*, const QString &url)
: _url(url) {
}

void PornTvBox::prepare() {
	setTitle(rpl::single(u"Porn TV"_q));

	addButton(tr::lng_box_ok(), [=] { closeBox(); });

	// Use a larger size for the TV
	int desiredWidth = st::boxWidth * 2; // Approximate double width
	int desiredHeight = 450;

	auto content = ::Ui::CreateChild<::Ui::RpWidget>(this);
	content->resize(desiredWidth, desiredHeight);
	
	_webview = std::make_unique<Webview::Window>(
		content,
		Webview::WindowConfig{
			.opaqueBg = st::boxBg->c,
		});
		
	if (auto w = _webview->widget()) {
		w->show();
		w->resize(content->size());
		
		content->sizeValue() | rpl::on_next([=](QSize size) {
			w->resize(size);
		}, content->lifetime());
	}
	
	_webview->navigate(_url);
	
	setDimensionsToContent(desiredWidth, content);
}

} // namespace Ayu::Ui
