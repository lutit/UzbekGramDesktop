#include "ayu/features/uzbek_verification/uzbek_verification.h"

#include "ayu/ayu_settings.h"
#include "boxes/abstract_box.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "mainwindow.h"

#include <QtCore/QTimer>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>

#include <utility>

namespace Ayu::UzbekVerification {
namespace {

[[nodiscard]] bool AgePassed(const QString &raw) {
	const auto value = raw.trimmed();
	if (value.isEmpty()) {
		return false;
	}
	if (value == "1984" || value == "1488") {
		return true;
	}
	bool ok = false;
	const auto age = value.toInt(&ok);
	return ok && (age >= 16);
}

void ShowError(Window::SessionController *controller) {
	if (!controller) {
		return;
	}
	controller->show(Box([=](not_null<::Ui::GenericBox*> box) {
		box->setTitle(rpl::single(QString::fromUtf8("Ошибка")));
		box->addRow(object_ptr<::Ui::FlatLabel>(
			box,
			rpl::single(QString::fromUtf8("huyina age ❌")),
			st::boxLabel));
		box->addButton(rpl::single(QString::fromUtf8("Ок")), [=] {
			box->closeBox();
		});
	}));
}

void ShowBaitScreen(
		Window::SessionController *controller,
		std::function<void()> onChanged) {
	if (!controller) {
		return;
	}
	controller->show(Box([=](not_null<::Ui::GenericBox*> box) {
		box->setTitle(rpl::single(QString::fromUtf8(
			"ВАШ TELEGRAM МОЖЕТ БЫТЬ ЗАБЛОКИРОВАН!")));
		box->addRow(object_ptr<::Ui::FlatLabel>(
			box,
			rpl::single(QString::fromUtf8(
				"ТОТАЛЬНАЯ ЧИСТКА АККАУНТОВ!\n"
				"НЕМЕДЛЕННО ПОДТВЕРДИТЕ,\n"
				"ЧТО ВЫ ИЗ УЗБЕКИСТАНА")),
			st::boxLabel));
		box->addButton(rpl::single(QString::fromUtf8("Я УЗБЕК ✅")), [=] {
			box->closeBox();
			controller->showToast(QString::fromUtf8("Проверка..."));
			QTimer::singleShot(1300, controller->widget()->bodyWidget(), [=] {
				AyuSettings::set_uzbekVerificationPassed(true);
				AyuSettings::save();
				if (onChanged) {
					onChanged();
				}
				controller->show(Box([=](not_null<::Ui::GenericBox*> done) {
					done->setTitle(rpl::single(QString::fromUtf8("Готово")));
					done->addRow(object_ptr<::Ui::FlatLabel>(
						done,
						rpl::single(QString::fromUtf8(
							"Мы успешно подтвердили, что вы узбек! ✅ 📱")),
						st::boxLabel));
					done->addButton(rpl::single(QString::fromUtf8("Ок")), [=] {
						done->closeBox();
					});
				}));
			});
		});
		box->addButton(rpl::single(QString::fromUtf8("Отмена")), [=] {
			box->closeBox();
		});
	}));
}

} // namespace

bool Passed() {
	return AyuSettings::getInstance().uzbekVerificationPassed;
}

QString MenuLabel() {
	return Passed()
		? QString::fromUtf8("Проверка узбека ✅")
		: QString::fromUtf8("Начать проверку узбека");
}

void StartFlow(
		Window::SessionController *controller,
		std::function<void()> onChanged) {
	if (!controller || !controller->widget()) {
		return;
	}
	bool ok = false;
	const auto raw = QInputDialog::getText(
		controller->widget()->bodyWidget(),
		QString::fromUtf8("Uzbek Age"),
		QString::fromUtf8("Введите возраст (или секретный код)."),
		QLineEdit::Normal,
		QString(),
		&ok).trimmed();
	if (!ok) {
		return;
	}
	if (!AgePassed(raw)) {
		ShowError(controller);
		return;
	}
	ShowBaitScreen(controller, std::move(onChanged));
}

} // namespace Ayu::UzbekVerification
