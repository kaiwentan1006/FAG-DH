/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/settings_menu/fa_settings_toggles.h"

#include "fa/settings/fa_settings.h"
#include "fa/lang/fa_lang.h"

#include "core/application.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_settings.h"

namespace FASettings {
namespace {

[[nodiscard]] rpl::producer<bool> AddToggle(
		not_null<Ui::VerticalLayout*> container,
		const QString &langKey,
		const QString &option) {
	return container->add(object_ptr<Ui::SettingsButton>(
		container,
		FAlang::RplTranslate(langKey),
		st::settingsButtonNoIcon
	))->toggleOn(
		rpl::single(JsonSettings::GetBool(option))
	)->toggledValue(
	) | rpl::filter([=](bool enabled) {
		return (enabled != JsonSettings::GetBool(option));
	});
}

} // namespace

void AddJsonToggle(
		not_null<Ui::VerticalLayout*> container,
		const QString &langKey,
		const QString &option) {
	AddToggle(
		container,
		langKey,
		option
	) | rpl::start_with_next([=](bool enabled) {
		JsonSettings::Set(option, enabled);
		JsonSettings::Write();
	}, container->lifetime());
}

void AddJsonRestartToggle(
		not_null<Ui::VerticalLayout*> container,
		not_null<Window::SessionController*> controller,
		const QString &langKey,
		const QString &option) {
	AddToggle(
		container,
		langKey,
		option
	) | rpl::start_with_next([=](bool enabled) {
		JsonSettings::Set(option, enabled);
		JsonSettings::Write();
		controller->show(Ui::MakeConfirmBox({
			.text = FAlang::RplTranslate(u"fa_setting_need_restart"_q),
			.confirmed = [=] { Core::Restart(); },
			.confirmText = FAlang::RplTranslate(u"fa_restart"_q),
		}));
	}, container->lifetime());
}

void AddJsonSlider(
		not_null<Ui::VerticalLayout*> container,
		const QString &option,
		Fn<QString(int)> label,
		Fn<void(int)> changed) {
	const auto range = JsonSettings::IntSettingRange(option);
	Assert(range.has_value());

	const auto labelWidget = container->add(
		object_ptr<Ui::LabelSimple>(
			container,
			st::settingsAudioVolumeLabel),
		st::settingsAudioVolumeLabelPadding);
	const auto slider = container->add(
		object_ptr<Ui::MediaSlider>(
			container,
			st::settingsAudioVolumeSlider),
		st::settingsAudioVolumeSliderPadding);
	const auto updateLabel = [=](int value) {
		labelWidget->setText(label(value));
	};
	const auto min = range->min;
	slider->resize(st::settingsAudioVolumeSlider.seekSize);
	slider->setPseudoDiscrete(
		range->max - min + 1,
		[=](int index) { return min + index; },
		JsonSettings::GetInt(option),
		[=](int value) {
			updateLabel(value);
			JsonSettings::Set(option, value);
			JsonSettings::Write();
			if (changed) {
				changed(value);
			}
		});
	updateLabel(JsonSettings::GetInt(option));
}

} // namespace FASettings
