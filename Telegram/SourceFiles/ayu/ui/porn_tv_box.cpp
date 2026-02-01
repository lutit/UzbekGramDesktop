#include "ayu/ui/porn_tv_box.h"

#include "ui/widgets/labels.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"
#include "core/application.h"
#include "window/window_controller.h"
#include "base/invoke_queued.h"
#include <QtCore/QTimer>

namespace Ayu::Ui {

PornTvBox::PornTvBox(QWidget*, const QString &url)
: _url(url) {
}

void PornTvBox::prepare() {
	setTitle(rpl::single(u"Porn TV"_q));
	addTopButton(st::boxTitleClose, [=] { closeBox(); });

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
	int desiredHeight = 700;
	
	if (const auto window = Core::App().activeWindow()) {
		desiredHeight = std::max(400, window->widget()->height() - 80);
	}

	auto content = ::Ui::CreateChild<::Ui::RpWidget>(this);
	content->resize(desiredWidth, desiredHeight);
	
	QTimer::singleShot(300, this, [=] {
		_webview = std::make_unique<Webview::Window>(
			content,
			Webview::WindowConfig{
				.opaqueBg = st::boxBg->c,
			});

		if (auto w = _webview->widget()) {
			w->show();
			w->resize(content->width(), content->height() - 15);

			content->sizeValue() | rpl::on_next([=](QSize size) {
				w->resize(size.width(), size.height() - 15);
			}, content->lifetime());
		}

		const auto updateUrl = [=] {
			if (!_webview) {
				return;
			}
			const auto toHex = [](QColor c) {
				return QString("%1%2%3")
					.arg(c.red(), 2, 16, QChar('0'))
					.arg(c.green(), 2, 16, QChar('0'))
					.arg(c.blue(), 2, 16, QChar('0'));
			};

			const auto bg = st::boxBg->c;
			const auto shadow = st::shadowFg->c;
			const auto shadowAlpha = shadow.alphaF();
			const auto mix = [&](int a, int b) {
				return int(a + (b - a) * shadowAlpha);
			};
			const auto border = QColor(
				mix(bg.red(), shadow.red()),
				mix(bg.green(), shadow.green()),
				mix(bg.blue(), shadow.blue())
			);

			const auto params = QString("bg_color=%1&text_color=%2&secondary_bg_color=%3&border_color=%4&button_color=%5")
				.arg(toHex(st::boxBg->c))
				.arg(toHex(st::windowFg->c))
				.arg(toHex(st::boxDividerBg->c))
				.arg(toHex(border))
				.arg(toHex(st::windowBgActive->c));

			auto fullUrl = _url;
			if (fullUrl.indexOf('?') >= 0) {
				fullUrl += '&' + params;
			} else {
				fullUrl += '?' + params;
			}
			_webview->navigate(fullUrl);
		};

		updateUrl();

		style::PaletteChanged(
		) | rpl::on_next(updateUrl, lifetime());
	});
	
	setDimensionsToContent(desiredWidth, content);
}

} // namespace Ayu::Ui
