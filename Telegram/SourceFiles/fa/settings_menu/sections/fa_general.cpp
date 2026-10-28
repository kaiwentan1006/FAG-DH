/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include <ui/boxes/single_choice_box.h>

#include "fa/settings/fa_settings.h"
#include "fa/settings_menu/sections/fa_general.h"
#include "fa/settings_menu/fa_settings_toggles.h"
#include "fa/utils/fa_capture_protection.h"
#include "fa/utils/fa_global_hotkeys.h"

#include "fa/lang/fa_lang.h"

#include "lang_auto.h"
#include "mainwindow.h"
#include "settings/settings_common.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/checkbox.h"
#include "ui/vertical_list.h"
#include "ui/boxes/confirm_box.h"
#include "boxes/connection_box.h"
#include "platform/platform_specific.h"
#include "window/window_session_controller.h"
#include "lang/lang_instance.h"
#include "core/application.h"
#include "storage/localstorage.h"
#include "data/data_session.h"
#include "main/main_session.h"
#include "styles/style_settings.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "apiwrap.h"
#include "api/api_blocked_peers.h"
#include "ui/widgets/continuous_sliders.h"

namespace Settings {

    rpl::producer<QString> FAGeneral::title() {
        return FAlang::RplTranslate(QString("fa_general"));
    }

    FAGeneral::FAGeneral(
            QWidget *parent,
            not_null<Window::SessionController *> controller)
            : Section(parent) {
        setupContent(controller);
    }

    void FAGeneral::SetupGeneral(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
        Ui::AddSubsectionTitle(container, FAlang::RplTranslate(QString("fa_general")));

		const auto toggles = {
			std::pair{ u"fa_show_seconds_message"_q, u"seconds_message"_q },
			std::pair{ u"fa_disable_ads"_q, u"disable_ads"_q },
			std::pair{ u"fa_show_start_token"_q, u"show_start_token"_q },
			std::pair{ u"fa_show_peer_ids"_q, u"show_peer_id"_q },
			std::pair{ u"fa_show_dc_ids"_q, u"show_dc_id"_q },
			std::pair{ u"fa_id_in_botapi_type"_q, u"show_id_botapi"_q },
			std::pair{ u"fa_local_tg_premium"_q, u"local_premium"_q },
			std::pair{ u"fa_show_registration_date"_q, u"show_registration_date"_q },
			std::pair{ u"fa_hide_phone_in_settings"_q, u"hide_phone_number"_q },
			std::pair{ u"fa_mark_read_after_action"_q, u"mark_read_after_action"_q },
		};
		for (const auto &[langKey, option] : toggles) {
			FASettings::AddJsonToggle(container, langKey, option);
		}
		FASettings::AddJsonRestartToggle(
			container,
			controller,
			u"fa_hide_stories"_q,
			u"hide_stories"_q);
    }

    void FAGeneral::SetupPrivacy(not_null<Ui::VerticalLayout *> container) {
		if (!FACaptureProtection::Supported()) {
			return;
		}
		Ui::AddSkip(container);
		Ui::AddDivider(container);
		Ui::AddSkip(container);
		Ui::AddSubsectionTitle(container, FAlang::RplTranslate(QString("fa_privacy")));
		FASettings::AddJsonToggle(
			container,
			u"fa_screen_capture_protection"_q,
			u"screen_capture_protection"_q);
		Ui::AddSkip(container);
		Ui::AddDividerText(
			container,
			FAlang::RplTranslate(QString("fa_screen_capture_protection_about")));
    }

    void FAGeneral::SetupGlobalShortcuts(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
		if (!FAGlobalHotkeys::Supported()) {
			return;
		}
		Ui::AddSkip(container);
		Ui::AddSubsectionTitle(container, FAlang::RplTranslate(u"fa_global_shortcuts"_q));
		const auto descriptor = FASettings::KeySequenceDescriptor{
			.valid = FAGlobalHotkeys::IsValid,
			.readable = FAGlobalHotkeys::ReadableText,
			.aboutLangKey = u"fa_global_shortcut_edit_about"_q,
		};
		FASettings::AddJsonKeySequence(
			container,
			controller,
			u"fa_global_shortcut_toggle_window"_q,
			FAGlobalHotkeys::ToggleWindowSettingKey(),
			descriptor);
		FASettings::AddJsonKeySequence(
			container,
			controller,
			u"fa_global_shortcut_toggle_capture_protection"_q,
			FAGlobalHotkeys::ToggleCaptureProtectionSettingKey(),
			descriptor);
		Ui::AddSkip(container);
		Ui::AddDividerText(
			container,
			FAlang::RplTranslate(u"fa_global_shortcuts_about"_q));
    }

    void FAGeneral::SetupFAGeneral(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
		Ui::AddSkip(container);
    	SetupGeneral(container, controller);
    	SetupPrivacy(container);
    	SetupGlobalShortcuts(container, controller);
    }

    void FAGeneral::setupContent(not_null<Window::SessionController *> controller) {
        const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

        SetupFAGeneral(content, controller);

        Ui::ResizeFitChild(this, content);
    }
} // namespace Settings

// thanks rabbitGram