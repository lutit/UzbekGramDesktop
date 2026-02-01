#include "ayu/ui/porn_tv_box.h"

#include "ui/widgets/labels.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"
#include "core/application.h"
#include "window/window_controller.h"
#include "base/invoke_queued.h"
#include "mainwindow.h"
#include <QtCore/QTimer>
#include <algorithm>

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

	auto content = ::Ui::CreateChild<::Ui::RpWidget>(this);

	if (const auto window = Core::App().activeWindow()) {
		const auto updateGeometry = [=](QSize size) {
			// Enforce vertical aspect ratio (9:16) for "Porn TV" style content
			const float aspectRatio = 9.0f / 16.0f;
			const int paddingY = 80;
			const int paddingX = 40;
			
			// Calculate available space
			const int maxH = std::max(500, size.height() - paddingY);
			const int maxW = std::max(360, size.width() - paddingX);
			
			// Target height: 90% of window height
			int h = std::clamp(int(size.height() * 0.9), 500, maxH);
			
			// Target width: Derived from height with aspect ratio
			int w = int(h * aspectRatio);
			
			// Constraint: Width must not exceed available width
			if (w > maxW) {
				w = maxW;
			}
			
			// Ensure strict minimums
			w = std::max(w, 360);
			h = std::max(h, 500);
			
			// Final safety clamp to window size
			w = std::min(w, size.width());
			h = std::min(h, size.height());
			
			if (content->width() != w || content->height() != h) {
				content->resize(w, h);
				setDimensions(w, h);
			}
		};

		updateGeometry(window->widget()->size());
		window->widget()->sizeValue() | rpl::on_next(updateGeometry, lifetime());

	} else {
		content->resize(380, 700);
		setDimensions(380, 700);
	}
	
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
}

} // namespace Ayu::Ui
