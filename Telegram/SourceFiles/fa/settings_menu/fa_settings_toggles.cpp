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
#include "lang_auto.h"
#include "settings/settings_common.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"

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
		// Follows the changes made elsewhere, like by a shortcut.
		rpl::single(
			JsonSettings::GetBool(option)
		) | rpl::then(
			JsonSettings::Events(option) | rpl::map([=](const QString &) {
				return JsonSettings::GetBool(option);
			})
		)
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

void AddJsonKeySequence(
		not_null<Ui::VerticalLayout*> container,
		not_null<Window::SessionController*> controller,
		const QString &langKey,
		const QString &option,
		KeySequenceDescriptor descriptor) {
	const auto readable = descriptor.readable;
	const auto valid = descriptor.valid;
	auto label = rpl::single(
		rpl::empty
	) | rpl::then(
		JsonSettings::Events(option) | rpl::to_empty
	) | rpl::map([=] {
		return readable(JsonSettings::GetString(option));
	});
	const auto about = descriptor.aboutLangKey;
	Settings::AddButtonWithLabel(
		container,
		FAlang::RplTranslate(langKey),
		std::move(label),
		st::settingsButtonNoIcon
	)->addClickHandler([=] {
		controller->show(Box([=](not_null<Ui::GenericBox*> box) {
			box->setTitle(FAlang::RplTranslate(langKey));
			const auto field = box->addRow(object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				FAlang::RplTranslate(u"fa_key_sequence_placeholder"_q),
				JsonSettings::GetString(option)));
			box->addRow(object_ptr<Ui::FlatLabel>(
				box,
				FAlang::RplTranslate(about),
				st::boxDividerLabel));
			box->setFocusCallback([=] {
				field->setFocusFast();
			});
			const auto save = [=] {
				const auto text = field->getLastText().trimmed();
				if (!text.isEmpty() && !valid(text)) {
					field->showError();
					return;
				}
				JsonSettings::Set(option, text);
				JsonSettings::Write();
				box->closeBox();
			};
			field->submits(
			) | rpl::start_with_next([=](Qt::KeyboardModifiers) {
				save();
			}, field->lifetime());
			box->addButton(tr::lng_settings_save(), save);
			box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
		}));
	});
}

} // namespace FASettings
