/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#pragma once

class HistoryItem;

namespace Main {
class Session;
} // namespace Main

namespace Ui {
class PopupMenu;
} // namespace Ui

namespace FAData {

// Device-only message actions: hiding and media blur toggling.
void AddMessageActions(
	not_null<Ui::PopupMenu*> menu,
	not_null<HistoryItem*> item);

void AddHideSelectedAction(
	not_null<Ui::PopupMenu*> menu,
	not_null<Main::Session*> session,
	MessageIdsList ids,
	Fn<void()> done);

} // namespace FAData
