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

namespace FAData {

// Per-account data that exists only on this device, stored encrypted
// in the account local storage next to the other tdata files.
class LocalStore final {
public:
	explicit LocalStore(not_null<Main::Session*> session);

	[[nodiscard]] bool isHidden(not_null<const HistoryItem*> item) const;
	[[nodiscard]] int hiddenCount(PeerId peer) const;
	[[nodiscard]] int hiddenCount() const;
	void hide(const MessageIdsList &ids);
	void unhide(PeerId peer);
	void unhideAll();

	// Senders hidden in one chat: everything they post there is hidden.
	[[nodiscard]] bool isSenderHidden(PeerId chat, PeerId sender) const;
	[[nodiscard]] int hiddenSendersCount(PeerId chat) const;
	[[nodiscard]] int hiddenSendersCount() const;
	void hideSender(PeerId chat, PeerId sender);
	void unhideSenders(PeerId chat);
	void unhideAllSenders();

	[[nodiscard]] bool isMediaBlurToggled(FullMsgId id) const;
	void toggleMediaBlur(FullMsgId id);

	[[nodiscard]] rpl::producer<> changes() const;

private:
	using SenderKey = std::pair<PeerId, PeerId>;

	void load();
	void save() const;
	void unhideWhere(Fn<bool(FullMsgId)> condition);
	void unhideSendersWhere(Fn<bool(const SenderKey&)> condition);
	void refreshViews(const std::vector<FullMsgId> &ids) const;
	void refreshChatViews(base::flat_set<PeerId> chats) const;
	void repaintMedia(FullMsgId id) const;
	void refreshAllMedia() const;

	const not_null<Main::Session*> _session;
	base::flat_set<FullMsgId> _hidden;
	base::flat_set<SenderKey> _hiddenSenders;
	base::flat_set<FullMsgId> _mediaBlurToggled;
	rpl::event_stream<> _changes;
	rpl::lifetime _lifetime;

};

[[nodiscard]] bool IsHiddenLocally(not_null<const HistoryItem*> item);

} // namespace FAData
