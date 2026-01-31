#include "ayu/ui/porn_tv_box.h"

#include "ui/widgets/labels.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"

namespace Ayu::Ui {

PornTvBox::PornTvBox(QWidget*, const QString &url)
: _url(url) {
}

void PornTvBox::prepare() {
	setTitle(rpl::single(u"Porn TV"_q));

	const auto st = &st::defaultBox;
	const auto style = lifetime().make_state<style::Box>(style::Box{
		.buttonPadding = QMargins(0, 0, 0, 0),
		.buttonHeight = 0,
		.margin = st->margin,
		.title = st->title,
		.bg = st->bg,
		.titleAdditionalFg = st->titleAdditionalFg,
		.shadowIgnoreTopSkip = st->shadowIgnoreTopSkip,
		.shadowIgnoreBottomSkip = st->shadowIgnoreBottomSkip,
	});
	setStyle(*style);
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
