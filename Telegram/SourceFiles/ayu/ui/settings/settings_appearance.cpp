// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_appearance.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/shalava_pro.h"
#include "ayu/ayu_ui_settings.h"
#include "ayu/ui/boxes/font_selector.h"
#include "ayu/ui/components/avatar_corners_preview.h"
#include "ayu/ui/components/icon_picker.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_ayu_utils.h"
#include "ayu/ui/settings/settings_main.h"
#include "inline_bots/bot_attach_web_view.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_ayu_icons.h"
#include "styles/style_ayu_styles.h"
#include "styles/style_dialogs.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/painter.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

bool HasDrawerBots(not_null<Window::SessionController*> controller) {
	// todo: maybe iterate through all accounts
	const auto bots = &controller->session().attachWebView();
	for (const auto &bot : bots->attachBots()) {
		if (!bot.inMainMenu || !bot.media) {
			continue;
		}
		return true;
	}
	return false;
}

void BuildAppIcon(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle({
		.id = u"ayu/appIcon"_q,
		.title = tr::ayu_AppIconHeader(),
	});

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<IconPicker>(ctx.container),
			.margin = st::settingsButtonNoIcon.padding,
		};
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	builder.addDivider();
	builder.addSkip();
	ayu.addSettingToggle({
		.id = u"ayu/hideNotificationBadge"_q,
		.title = tr::ayu_HideNotificationBadge(),
		.getter = &AyuSettings::hideNotificationBadge,
		.setter = &AyuSettings::setHideNotificationBadge,
	});
	builder.addSkip();
	builder.addDividerText(tr::ayu_HideNotificationBadgeDescription());
	builder.addSkip();
#else
    builder.addDivider();
    builder.addSkip();
#endif
}

void BuildAvatarCorners(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	auto *settings = &AyuSettings::getInstance();
	const auto controller = builder.controller();

	const auto mapRadius = [](int val)
	{
		if (val == 0) {
			return tr::ayu_AvatarCornersSquare(tr::now).toUpper();
		} else if (val == AyuUiSettings::kMaxAvatarCorners) {
			return tr::ayu_AvatarCornersCircle(tr::now).toUpper();
		}
		return QString::number(val);
	};

	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		const auto container = ctx.container;
		auto title = object_ptr<Ui::FlatLabel>(
			container,
			tr::ayu_AvatarCorners(),
			st::defaultSubsectionTitle);
		const auto titleRaw = title.data();

		const auto badge = Ui::CreateChild<Ui::PaddingWrap<Ui::FlatLabel>>(
			container,
			object_ptr<Ui::FlatLabel>(
				container,
				settings->avatarCornersValue() | rpl::map(mapRadius),
				st::settingsPremiumNewBadge),
			st::ayuBetaBadgePadding);
		badge->show();
		badge->setAttribute(Qt::WA_TransparentForMouseEvents);
		badge->paintRequest() | rpl::on_next([=] {
			auto p = QPainter(badge);
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgActive);
			const auto r = st::ayuBetaBadgePadding.left();
			p.drawRoundedRect(badge->rect(), r, r);
		}, badge->lifetime());

		titleRaw->geometryValue() | rpl::on_next([=](QRect geometry) {
			badge->moveToLeft(
				geometry.x()
					+ titleRaw->textMaxWidth()
					+ st::settingsPremiumNewBadgePosition.x(),
				geometry.y()
					+ (geometry.height() - badge->height()) / 2);
		}, badge->lifetime());

		return {
			.widget = std::move(title),
			.margin = st::defaultSubsectionTitlePadding,
		};
	}, [] {
		return SearchEntry{
			.id = u"ayu/avatarCorners"_q,
			.title = tr::ayu_AvatarCorners(tr::now),
		};
	});

	auto *previewRaw = static_cast<AvatarCornersPreview*>(nullptr);
	builder.add([&](const Builder::WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto preview = object_ptr<AvatarCornersPreview>(
			ctx.container,
			controller);
		previewRaw = preview.data();
		const auto vMargin = st::settingsButtonNoIcon.padding
			- st::defaultDialogRow.padding;
		return {
			.widget = std::move(preview),
			.margin = QMargins(0, vMargin.top(), 0, vMargin.bottom()),
		};
	});

	ayu.addSlider({
		.id = u"ayu/avatarCornersSlider"_q,
		.title = rpl::single(QString()),
		.showTitle = false,
		.steps = AyuUiSettings::kMaxAvatarCorners + 1,
		.current = settings->avatarCorners(),
		.onChanged = [=](int val) {
			AyuSettings::getInstance().setAvatarCorners(val);
			if (previewRaw) {
				previewRaw->update();
			}
		},
		.onFinalChanged = [=](int val) {
			AyuSettings::getInstance().setAvatarCorners(val);
			ShowRestartPrompt(controller);
		},
	});

	ayu.addSettingToggle({
		.id = u"ayu/singleCornerRadius"_q,
		.title = tr::ayu_SingleCornerRadius(),
		.getter = &AyuSettings::singleCornerRadius,
		.setter = &AyuSettings::setSingleCornerRadius,
	});

	builder.addSkip();
	builder.addDividerText(tr::ayu_SingleCornerRadiusDescription());
	builder.addSkip();
}

void BuildAppearance(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	auto *settings = &AyuSettings::getInstance();

	builder.addSubsectionTitle(tr::ayu_CategoryAppearance());

	ayu.addSettingToggle({
		.id = u"ayu/materialSwitches"_q,
		.altIds = { u"ayu/newSwitchStyle"_q },
		.title = tr::ayu_MaterialSwitches(),
		.getter = &AyuSettings::materialSwitches,
		.setter = &AyuSettings::setMaterialSwitches,
	});
	ayu.addSettingToggle({
		.id = u"ayu/disableCustomBackgrounds"_q,
		.altIds = { u"ayu/customThemes"_q },
		.title = tr::ayu_DisableCustomBackgrounds(),
		.getter = &AyuSettings::disableCustomBackgrounds,
		.setter = &AyuSettings::setDisableCustomBackgrounds,
	});

	ayu.addSettingToggle({
		.id = u"ayu/hidePremiumStatuses"_q,
		.title = tr::ayu_HidePremiumStatuses(),
		.getter = &AyuSettings::hidePremiumStatuses,
		.setter = &AyuSettings::setHidePremiumStatuses,
	});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"ayu/monoFont"_q,
		.title = tr::ayu_MonospaceFont(),
		.st = &st::settingsButtonNoIcon,
		.label = rpl::single(
			settings->monoFont().isEmpty()
				? tr::ayu_FontDefault(tr::now)
				: settings->monoFont()),
		.onClick = [=] {
			AyuUi::FontSelectorBox::Show(
				controller,
				[=](const QString &font) {
					AyuSettings::getInstance().setMonoFont(font);
				});
		},
	});

	ayu.addSectionDivider();
}

void BuildChatFolders(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::ayu_ChatFoldersHeader());

	ayu.addSettingToggle({
		.id = u"ayu/hideNotificationCounters"_q,
		.altIds = { u"ayu/tabCounter"_q },
		.title = tr::ayu_HideNotificationCounters(),
		.getter = &AyuSettings::hideNotificationCounters,
		.setter = &AyuSettings::setHideNotificationCounters,
	});
	ayu.addSettingToggle({
		.id = u"ayu/hideAllChatsFolder"_q,
		.altIds = { u"ayu/hideAllChats"_q },
		.title = tr::ayu_HideAllChats(),
		.getter = &AyuSettings::hideAllChatsFolder,
		.setter = &AyuSettings::setHideAllChatsFolder,
	});

	ayu.addSectionDivider();
}

void BuildTrayElements(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::ayu_TrayElementsHeader());

	ayu.addSettingToggle({
		.id = u"ayu/showGhostToggleInTray"_q,
		.title = tr::ayu_EnableGhostModeTray(),
		.getter = &AyuSettings::showGhostToggleInTray,
		.setter = &AyuSettings::setShowGhostToggleInTray,
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	ayu.addSettingToggle({
		.id = u"ayu/showStreamerToggleInTray"_q,
		.title = tr::ayu_EnableStreamerModeTray(),
		.getter = &AyuSettings::showStreamerToggleInTray,
		.setter = &AyuSettings::setShowStreamerToggleInTray,
	});
#endif

	ayu.addSectionDivider();
}

void BuildDrawerElements(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::ayu_DrawerElementsHeader());

	ayu.addSettingToggle({
		.id = u"ayu/showMyProfileInDrawer"_q,
		.title = tr::lng_menu_my_profile(),
		.getter = &AyuSettings::showMyProfileInDrawer,
		.setter = &AyuSettings::setShowMyProfileInDrawer,
		.icon = { &st::menuIconProfile },
	});

	const auto controller = builder.controller();
	if (controller && HasDrawerBots(controller)) {
		ayu.addSettingToggle({
			.id = u"ayu/showBotsInDrawer"_q,
			.title = tr::lng_filters_type_bots(),
			.getter = &AyuSettings::showBotsInDrawer,
			.setter = &AyuSettings::setShowBotsInDrawer,
			.icon = { &st::menuIconBot },
		});
	}

	ayu.addSettingToggle({
		.id = u"ayu/showNewGroupInDrawer"_q,
		.title = tr::lng_create_group_title(),
		.getter = &AyuSettings::showNewGroupInDrawer,
		.setter = &AyuSettings::setShowNewGroupInDrawer,
		.icon = { &st::menuIconGroups },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showNewChannelInDrawer"_q,
		.title = tr::lng_create_channel_title(),
		.getter = &AyuSettings::showNewChannelInDrawer,
		.setter = &AyuSettings::setShowNewChannelInDrawer,
		.icon = { &st::menuIconChannel },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showContactsInDrawer"_q,
		.title = tr::lng_menu_contacts(),
		.getter = &AyuSettings::showContactsInDrawer,
		.setter = &AyuSettings::setShowContactsInDrawer,
		.icon = { &st::menuIconUserShow },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showCallsInDrawer"_q,
		.title = tr::lng_menu_calls(),
		.getter = &AyuSettings::showCallsInDrawer,
		.setter = &AyuSettings::setShowCallsInDrawer,
		.icon = { &st::menuIconPhone },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showSavedMessagesInDrawer"_q,
		.title = tr::lng_saved_messages(),
		.getter = &AyuSettings::showSavedMessagesInDrawer,
		.setter = &AyuSettings::setShowSavedMessagesInDrawer,
		.icon = { &st::menuIconSavedMessages },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showLReadToggleInDrawer"_q,
		.title = tr::ayu_LReadMessages(),
		.getter = &AyuSettings::showLReadToggleInDrawer,
		.setter = &AyuSettings::setShowLReadToggleInDrawer,
		.icon = { &st::ayuLReadMenuIcon },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showSReadToggleInDrawer"_q,
		.title = tr::ayu_SReadMessages(),
		.getter = &AyuSettings::showSReadToggleInDrawer,
		.setter = &AyuSettings::setShowSReadToggleInDrawer,
		.icon = { &st::ayuSReadMenuIcon },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showNightModeToggleInDrawer"_q,
		.title = tr::lng_menu_night_mode(),
		.getter = &AyuSettings::showNightModeToggleInDrawer,
		.setter = &AyuSettings::setShowNightModeToggleInDrawer,
		.icon = { &st::menuIconNightMode },
	});
	ayu.addSettingToggle({
		.id = u"ayu/showGhostToggleInDrawer"_q,
		.title = tr::ayu_GhostModeToggle(),
		.getter = &AyuSettings::showGhostToggleInDrawer,
		.setter = &AyuSettings::setShowGhostToggleInDrawer,
		.icon = { &st::ayuGhostIcon },
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	ayu.addSettingToggle({
		.id = u"ayu/showStreamerToggleInDrawer"_q,
		.title = tr::ayu_StreamerModeToggle(),
		.getter = &AyuSettings::showStreamerToggleInDrawer,
		.setter = &AyuSettings::setShowStreamerToggleInDrawer,
		.icon = { &st::ayuStreamerModeMenuIcon },
	});
#endif

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuAppearance::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_CategoryAppearance,
	.icon = &st::menuIconPalette,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	builder.addSkip();
	BuildAppIcon(builder, ayu);
	BuildAvatarCorners(builder, ayu);
	BuildAppearance(builder, ayu);
	BuildChatFolders(builder, ayu);
	BuildTrayElements(builder, ayu);
	BuildDrawerElements(builder, ayu);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> AyuAppearance::title() {
	return tr::ayu_CategoryAppearance();
}

AyuAppearance::AyuAppearance(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void SetupShalavaSettings(not_null<Ui::VerticalLayout*> container, not_null<Window::SessionController*> controller) {
	auto *settings = &AyuSettings::getInstance();

	AddSubsectionTitle(container, rpl::single(u"Shalava Mod"_q));

	const auto toggleShalava = [=](int mode) {
		const auto current = AyuSettings::getInstance().shalavaMode;
		const auto next = (current == mode) ? 0 : mode;

		auto activate = [=](int m) {
			AyuSettings::set_shalavaMode(m);
			AyuSettings::set_shalavaModePro(m == 3);
			AyuSettings::save();
		};

		if (next == 3) {
			if (!Ayu::ShalavaPro::instance().isUnlocked()) {
				Ayu::ShalavaPro::instance().showUnlockPopup(controller);
				return;
			}
			if (!settings->shalavaEpilepsyWarningShown && !settings->shalavaSafeMode) {
				activate(3);
			} else {
				activate(3);
			}
		} else {
			activate(next);
		}
	};

	AddButtonWithIcon(
		container,
		rpl::single(u"Shalava Mod"_q),
		st::settingsButtonNoIcon
	)->toggleOn(
		AyuSettings::get_shalavaModeReactive() | rpl::map([](int mode) { return mode == 1; })
	)->setClickedCallback([=] { toggleShalava(1); });

	AddButtonWithIcon(
		container,
		rpl::single(u"Super Shalava"_q),
		st::settingsButtonNoIcon
	)->toggleOn(
		AyuSettings::get_shalavaModeReactive() | rpl::map([](int mode) { return mode == 2; })
	)->setClickedCallback([=] { toggleShalava(2); });

	AddButtonWithIcon(
		container,
		rpl::single(u"Ultra Shalava"_q),
		st::settingsButtonNoIcon
	)->toggleOn(
		AyuSettings::get_shalavaModeReactive() | rpl::map([](int mode) { return mode == 3; })
	)->setClickedCallback([=] { toggleShalava(3); });

	AddButtonWithIcon(
		container,
		rpl::single(u"Safe Mode"_q),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->shalavaSafeMode)
	)->toggledValue(
	) | rpl::on_next([=](bool val) {
		AyuSettings::set_shalavaSafeMode(val);
		AyuSettings::save();
	}, container->lifetime());

	container->add(
		object_ptr<Ui::SettingsButton>(container,
						   rpl::single(u"Particle Limit"_q),
						   st::settingsButtonNoIcon)
	)->setAttribute(Qt::WA_TransparentForMouseEvents);

	auto particleLimit = MakeSliderWithLabel(
		container,
		st::defaultContinuousSlider,
		st::settingsScaleLabel,
		0,
		st::settingsScaleLabel.style.font->width("500"));
	container->add(std::move(particleLimit.widget), st::settingsCheckboxPadding);
	
	const auto slider = particleLimit.slider;
	const auto label = particleLimit.label;

	const auto updateLabel = [=](int amount) {
		label->setText(QString::number(amount));
	};
	updateLabel(settings->shalavaParticleLimit);

	slider->setPseudoDiscrete(
		500 + 1,
		[=](int amount) { return amount; },
		settings->shalavaParticleLimit,
		[=](int amount) { updateLabel(amount); },
		[=](int amount) {
			updateLabel(amount);
			AyuSettings::set_shalavaParticleLimit(amount);
			AyuSettings::save();
		});

	AddButtonWithIcon(
		container,
		rpl::single(u"Text Overlays"_q),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->shalavaTextOverlay)
	)->toggledValue(
	) | rpl::on_next([=](bool val) {
		AyuSettings::set_shalavaTextOverlay(val);
		AyuSettings::save();
	}, container->lifetime());

	AddButtonWithIcon(
		container,
		rpl::single(u"Overlay Captures Mouse"_q),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->shalavaOverlayCapturesMouse)
	)->toggledValue(
	) | rpl::on_next([=](bool val) {
		AyuSettings::set_shalavaOverlayCapturesMouse(val);
		AyuSettings::save();
	}, container->lifetime());

	AddButtonWithIcon(
		container,
		rpl::single(u"Reset Epilepsy Warning"_q),
		st::settingsButtonNoIcon
	)->setClickedCallback([=] {
		AyuSettings::set_shalavaEpilepsyWarningShown(false);
		AyuSettings::save();
	});

	AddSkip(container);
	AddDivider(container);
	AddSkip(container);
}

void SetupAppearance(not_null<Ui::VerticalLayout*> container, not_null<Window::SessionController*> controller) {
	auto *settings = &AyuSettings::getInstance();

	AddSubsectionTitle(container, tr::ayu_CategoryAppearance());

	AddButtonWithIcon(
		container,
		tr::ayu_MaterialSwitches(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->materialSwitches)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->materialSwitches);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_materialSwitches(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::ayu_RemoveMessageTail(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->removeMessageTail)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->removeMessageTail);
		}) | on_next(
		[=](bool enabled)
		{
			AyuSettings::set_removeMessageTail(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::ayu_DisableCustomBackgrounds(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->disableCustomBackgrounds)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->disableCustomBackgrounds);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_disableCustomBackgrounds(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	const auto monoButton = AddButtonWithLabel(
		container,
		tr::ayu_MonospaceFont(),
		rpl::single(
			settings->monoFont.isEmpty() ? tr::ayu_FontDefault(tr::now) : settings->monoFont
		),
		st::settingsButtonNoIcon);
	const auto monoGuard = Ui::CreateChild<base::binary_guard>(monoButton.get());

	monoButton->addClickHandler(
		[=]
		{
			*monoGuard = AyuUi::FontSelectorBox::Show(
				controller,
				[=](QString font)
				{
					AyuSettings::set_monoFont(std::move(font));
					AyuSettings::save();
				});
		});

	AddSkip(container);
	AddDivider(container);
	AddSkip(container);
}

void SetupChatFolders(not_null<Ui::VerticalLayout*> container) {
	auto *settings = &AyuSettings::getInstance();

	AddSubsectionTitle(container, tr::ayu_ChatFoldersHeader());

	AddButtonWithIcon(
		container,
		tr::ayu_HideNotificationCounters(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->hideNotificationCounters)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->hideNotificationCounters);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_hideNotificationCounters(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::ayu_HideAllChats(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->hideAllChatsFolder)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->hideAllChatsFolder);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_hideAllChatsFolder(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddSkip(container);
	AddDivider(container);
	AddSkip(container);
}

void SetupDrawerElements(not_null<Ui::VerticalLayout*> container, not_null<Window::SessionController*> controller) {
	auto *settings = &AyuSettings::getInstance();

	AddSubsectionTitle(container, tr::ayu_DrawerElementsHeader());

	AddButtonWithIcon(
		container,
		tr::lng_menu_my_profile(),
		st::settingsButton,
		{&st::menuIconProfile}
	)->toggleOn(
		rpl::single(settings->showMyProfileInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showMyProfileInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showMyProfileInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	if (HasDrawerBots(controller)) {
		AddButtonWithIcon(
			container,
			tr::lng_filters_type_bots(),
			st::settingsButton,
			{&st::menuIconBot}
		)->toggleOn(
			rpl::single(settings->showBotsInDrawer)
		)->toggledValue(
		) | rpl::filter(
			[=](bool enabled)
			{
				return (enabled != settings->showBotsInDrawer);
			}) | rpl::on_next(
			[=](bool enabled)
			{
				AyuSettings::set_showBotsInDrawer(enabled);
				AyuSettings::save();
			},
			container->lifetime());
	}

	AddButtonWithIcon(
		container,
		tr::lng_create_group_title(),
		st::settingsButton,
		{&st::menuIconGroups}
	)->toggleOn(
		rpl::single(settings->showNewGroupInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showNewGroupInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showNewGroupInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::lng_create_channel_title(),
		st::settingsButton,
		{&st::menuIconChannel}
	)->toggleOn(
		rpl::single(settings->showNewChannelInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showNewChannelInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showNewChannelInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::lng_menu_contacts(),
		st::settingsButton,
		{&st::menuIconUserShow}
	)->toggleOn(
		rpl::single(settings->showContactsInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showContactsInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showContactsInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::lng_menu_calls(),
		st::settingsButton,
		{&st::menuIconPhone}
	)->toggleOn(
		rpl::single(settings->showCallsInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showCallsInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showCallsInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::lng_saved_messages(),
		st::settingsButton,
		{&st::menuIconSavedMessages}
	)->toggleOn(
		rpl::single(settings->showSavedMessagesInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showSavedMessagesInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showSavedMessagesInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::ayu_LReadMessages(),
		st::settingsButton,
		{&st::ayuLReadMenuIcon}
	)->toggleOn(
		rpl::single(settings->showLReadToggleInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showLReadToggleInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showLReadToggleInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::ayu_SReadMessages(),
		st::settingsButton,
		{&st::ayuSReadMenuIcon}
	)->toggleOn(
		rpl::single(settings->showSReadToggleInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showSReadToggleInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showSReadToggleInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::lng_menu_night_mode(),
		st::settingsButton,
		{&st::menuIconNightMode}
	)->toggleOn(
		rpl::single(settings->showNightModeToggleInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showNightModeToggleInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showNightModeToggleInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

	AddButtonWithIcon(
		container,
		tr::ayu_GhostModeToggle(),
		st::settingsButton,
		{&st::ayuGhostIcon}
	)->toggleOn(
		rpl::single(settings->showGhostToggleInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showGhostToggleInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showGhostToggleInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());

#ifdef WIN32
	AddButtonWithIcon(
		container,
		tr::ayu_StreamerModeToggle(),
		st::settingsButton,
		{&st::ayuStreamerModeMenuIcon}
	)->toggleOn(
		rpl::single(settings->showStreamerToggleInDrawer)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showStreamerToggleInDrawer);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showStreamerToggleInDrawer(enabled);
			AyuSettings::save();
		},
		container->lifetime());
#endif

	AddSkip(container);
}

void SetupTrayElements(not_null<Ui::VerticalLayout*> container) {
	auto *settings = &AyuSettings::getInstance();

	AddSubsectionTitle(container, tr::ayu_TrayElementsHeader());

	AddButtonWithIcon(
		container,
		tr::ayu_EnableGhostModeTray(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->showGhostToggleInTray)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showGhostToggleInTray);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showGhostToggleInTray(enabled);
			AyuSettings::save();
		},
		container->lifetime());

#ifdef WIN32
	AddButtonWithIcon(
		container,
		tr::ayu_EnableStreamerModeTray(),
		st::settingsButtonNoIcon
	)->toggleOn(
		rpl::single(settings->showStreamerToggleInTray)
	)->toggledValue(
	) | rpl::filter(
		[=](bool enabled)
		{
			return (enabled != settings->showStreamerToggleInTray);
		}) | rpl::on_next(
		[=](bool enabled)
		{
			AyuSettings::set_showStreamerToggleInTray(enabled);
			AyuSettings::save();
		},
		container->lifetime());
#endif

	AddSkip(container);
	AddDivider(container);
	AddSkip(container);
}

void AyuAppearance::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	AddSkip(content);

	SetupAppIcon(content);
	SetupAppearance(content, controller);
	SetupChatFolders(content);
	SetupTrayElements(content);
	SetupShalavaSettings(content, controller);
	SetupDrawerElements(content, controller);
	AddSkip(content);

	ResizeFitChild(this, content);
=======
void AyuAppearance::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
>>>>>>> refs/tags/v6.7.8
}

Type AyuAppearanceId() {
	return AyuAppearance::Id();
}

} // namespace Settings
