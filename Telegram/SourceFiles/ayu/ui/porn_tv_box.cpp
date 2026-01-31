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
	setNoContentMargin(true);

	// Use a vertical layout size optimized for mobile content
	int desiredWidth = 380; 
	int desiredHeight = 680;

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
