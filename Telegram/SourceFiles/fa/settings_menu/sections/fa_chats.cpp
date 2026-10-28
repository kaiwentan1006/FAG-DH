/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include <ui/boxes/single_choice_box.h>

#include "fa/settings/fa_settings.h"
#include "fa/settings_menu/sections/fa_chats.h"
#include "fa/settings_menu/fa_settings_toggles.h"
#include "fa/data/fa_local_store.h"
#include "fa/ui/fa_autoscroll.h"

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

    rpl::producer<QString> FAChats::title() {
        return FAlang::RplTranslate(QString("fa_chats"));
    }

    FAChats::FAChats(
            QWidget *parent,
            not_null<Window::SessionController *> controller)
            : Section(parent) {
        setupContent(controller);
    }

    void FAChats::SetupChats(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
        Ui::AddSubsectionTitle(container, FAlang::RplTranslate(QString("fa_chats")));

		FASettings::AddJsonSlider(
			container,
			u"recent_stickers_limit"_q,
			[](int value) {
				return (value == 0)
					? FAlang::Translate(u"fa_recent_stickers_hidden"_q)
					: FAlang::Translate(u"fa_recent_stickers"_q).arg(value);
			});
		const auto toggles = {
			std::pair{ u"fa_disable_custom_chat_background"_q, u"disable_custom_chat_background"_q },
			std::pair{ u"fa_hide_open_webapp_button_chatlist"_q, u"hide_open_webapp_button_chatlist"_q },
			std::pair{ u"fa_show_discuss_button"_q, u"show_discuss_button"_q },
			std::pair{ u"fa_show_message_details"_q, u"show_message_details"_q },
		};
		for (const auto &[langKey, option] : toggles) {
			FASettings::AddJsonToggle(container, langKey, option);
		}
		FASettings::AddJsonRestartToggle(
			container,
			controller,
			u"fa_hide_all_chats_folder"_q,
			u"hide_all_chats_folder"_q);
    }

    void FAChats::SetupMediaBlur(not_null<Ui::VerticalLayout *> container) {
		Ui::AddSkip(container);
		Ui::AddDivider(container);
		Ui::AddSkip(container);
		Ui::AddSubsectionTitle(container, FAlang::RplTranslate(u"fa_media_blur"_q));
		const auto toggles = {
			std::pair{ u"fa_blur_photos"_q, u"blur_photos"_q },
			std::pair{ u"fa_blur_videos"_q, u"blur_videos"_q },
			std::pair{ u"fa_blur_stickers"_q, u"blur_stickers"_q },
			std::pair{ u"fa_blur_files"_q, u"blur_files"_q },
		};
		for (const auto &[langKey, option] : toggles) {
			FASettings::AddJsonToggle(container, langKey, option);
		}
		FASettings::AddJsonSlider(
			container,
			u"blur_strength"_q,
			[](int value) {
				return FAlang::Translate(u"fa_blur_strength"_q).arg(value);
			});
		Ui::AddDividerText(
			container,
			FAlang::RplTranslate(u"fa_media_blur_about"_q));
    }

    void FAChats::SetupAutoscroll(not_null<Ui::VerticalLayout *> container) {
		Ui::AddSkip(container);
		Ui::AddSubsectionTitle(container, FAlang::RplTranslate(u"fa_autoscroll"_q));
		FASettings::AddJsonToggle(
			container,
			u"fa_autoscroll_enabled"_q,
			FAAutoscroll::EnabledSettingKey());
		FASettings::AddJsonSlider(
			container,
			FAAutoscroll::SpeedSettingKey(),
			[](int value) {
				return FAlang::Translate(u"fa_autoscroll_speed"_q).arg(value);
			});
		FASettings::AddJsonSlider(
			container,
			FAAutoscroll::DeadZoneSettingKey(),
			[](int value) {
				return FAlang::Translate(u"fa_autoscroll_dead_zone"_q).arg(value);
			});
		Ui::AddDividerText(
			container,
			FAlang::RplTranslate(u"fa_autoscroll_about"_q));
    }

    void FAChats::SetupHiddenMessages(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
		Ui::AddSkip(container);
		Ui::AddSubsectionTitle(container, FAlang::RplTranslate(u"fa_hidden_messages"_q));
		const auto store = &controller->session().faLocalStore();
		auto count = rpl::single(
			rpl::empty
		) | rpl::then(
			store->changes()
		) | rpl::map([=] {
			return QString::number(store->hiddenCount());
		});
		AddButtonWithLabel(
			container,
			FAlang::RplTranslate(u"fa_unhide_all_messages"_q),
			std::move(count),
			st::settingsButtonNoIcon
		)->addClickHandler([=] {
			if (!store->hiddenCount()) {
				return;
			}
			controller->show(Ui::MakeConfirmBox({
				.text = FAlang::RplTranslate(u"fa_unhide_all_messages_sure"_q),
				.confirmed = [=](Fn<void()> close) {
					store->unhideAll();
					close();
				},
				.confirmText = FAlang::RplTranslate(u"fa_unhide"_q),
			}));
		});
		auto senders = rpl::single(
			rpl::empty
		) | rpl::then(
			store->changes()
		) | rpl::map([=] {
			return QString::number(store->hiddenSendersCount());
		});
		AddButtonWithLabel(
			container,
			FAlang::RplTranslate(u"fa_unhide_all_senders"_q),
			std::move(senders),
			st::settingsButtonNoIcon
		)->addClickHandler([=] {
			if (!store->hiddenSendersCount()) {
				return;
			}
			controller->show(Ui::MakeConfirmBox({
				.text = FAlang::RplTranslate(u"fa_unhide_all_senders_sure"_q),
				.confirmed = [=](Fn<void()> close) {
					store->unhideAllSenders();
					close();
				},
				.confirmText = FAlang::RplTranslate(u"fa_unhide"_q),
			}));
		});
		FASettings::AddJsonToggle(
			container,
			u"fa_hide_membership_messages"_q,
			u"hide_membership_messages"_q);
		FASettings::AddJsonToggle(
			container,
			u"fa_hide_unsupported_messages"_q,
			u"hide_unsupported_messages"_q);
		Ui::AddSkip(container);
		Ui::AddDividerText(
			container,
			FAlang::RplTranslate(u"fa_hidden_messages_about"_q));
    }

    void FAChats::SetupFAChats(not_null<Ui::VerticalLayout *> container, not_null<Window::SessionController *> controller) {
		Ui::AddSkip(container);
    	SetupChats(container, controller);
		SetupMediaBlur(container);
		SetupAutoscroll(container);
		SetupHiddenMessages(container, controller);
    }

    void FAChats::setupContent(not_null<Window::SessionController *> controller) {
        const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

        SetupFAChats(content, controller);

        Ui::ResizeFitChild(this, content);
    }
} // namespace Settings

// thanks rabbitGram