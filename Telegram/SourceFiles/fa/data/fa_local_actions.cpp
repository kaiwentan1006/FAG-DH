/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/data/fa_local_actions.h"

#include "fa/data/fa_local_store.h"
#include "fa/lang/fa_lang.h"
#include "fa/ui/fa_media_blur.h"

#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "ui/widgets/popup_menu.h"
#include "styles/style_menu_icons.h"

namespace FAData {

void AddMessageActions(
		not_null<Ui::PopupMenu*> menu,
		not_null<HistoryItem*> item) {
	const auto weak = base::make_weak(&item->history()->session());
	const auto id = item->fullId();
	if (FAMediaBlur::HasBlurrableMedia(item)) {
		const auto blurred = FAMediaBlur::IsItemMediaBlurred(item);
		menu->addAction(
			FAlang::Translate(blurred
				? u"fa_unblur_media"_q
				: u"fa_blur_media"_q),
			[=] {
				if (const auto strong = weak.get()) {
					strong->faLocalStore().toggleMediaBlur(id);
				}
			},
			blurred ? &st::menuIconSpoilerOff : &st::menuIconSpoiler);
	}
	if (IsServerMsgId(id.msg)) {
		menu->addAction(
			FAlang::Translate(u"fa_hide_locally"_q),
			[=] {
				if (const auto strong = weak.get()) {
					strong->faLocalStore().hide({ id });
				}
			},
			&st::menuIconDisable);
	}
}

void AddHideSelectedAction(
		not_null<Ui::PopupMenu*> menu,
		not_null<Main::Session*> session,
		MessageIdsList ids,
		Fn<void()> done) {
	if (ids.empty()) {
		return;
	}
	const auto weak = base::make_weak(session);
	menu->addAction(
		FAlang::Translate(u"fa_hide_selected_locally"_q),
		[=] {
			if (const auto strong = weak.get()) {
				strong->faLocalStore().hide(ids);
			}
			if (done) {
				done();
			}
		},
		&st::menuIconDisable);
}

} // namespace FAData
