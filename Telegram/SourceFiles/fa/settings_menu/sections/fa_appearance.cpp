/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include <ui/boxes/single_choice_box.h>

#include "fa/settings/fa_settings.h"
#include "fa/settings_menu/sections/fa_appearance.h"
#include "fa/settings_menu/fa_settings_toggles.h"
#include "fa/ui/previews.h"

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

    rpl::producer<QString> FAAppearance::title() {
        return FAlang::RplTranslate(QString("fa_appearance"));
    }

    FAAppearance::FAAppearance(
            QWidget *parent,
            not_null<Window::SessionController *> controller)
            : Section(parent) {
        setupContent(controller);
    }

    void FAAppearance::SetupAppearance(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
        Ui::AddSubsectionTitle(container, FAlang::RplTranslate(("fa_appearance")));

		const auto roundnessPreview = container->add(
			object_ptr<RoundnessPreview>(container),
			st::defaultSubsectionTitlePadding);
		FASettings::AddJsonSlider(
			container,
			u"roundness"_q,
			[](int value) {
				return FAlang::Translate(u"fa_rounding"_q).arg(value);
			},
			[=](int) { roundnessPreview->repaint(); });
        Ui::AddDivider(container);
		FASettings::AddJsonRestartToggle(
			container,
			controller,
			u"fa_use_default_rounding"_q,
			u"use_default_rounding"_q);
		FASettings::AddJsonToggle(
			container,
			u"fa_force_snow"_q,
			u"force_snow"_q);
    }

    void FAAppearance::SetupFAAppearance(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
		Ui::AddSkip(container);
    	SetupAppearance(container, controller);
    }

    void FAAppearance::setupContent(not_null<Window::SessionController *> controller) {
        const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

        SetupFAAppearance(content, controller);

        Ui::ResizeFitChild(this, content);
    }
} // namespace Settings

// thanks rabbitGram