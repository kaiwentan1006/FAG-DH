/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/data/fa_local_store.h"

#include "fa/settings/fa_settings.h"
#include "fa/ui/fa_media_blur.h"

#include "data/data_forum_topic.h"
#include "data/data_groups.h"
#include "data/data_histories.h"
#include "data/data_peer.h"
#include "data/data_saved_sublist.h"
#include "data/data_session.h"
#include "dialogs/ui/dialogs_message_view.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "main/main_session.h"
#include "storage/storage_account.h"

#include <QtCore/QDataStream>

namespace FAData {
namespace {

// Bump when the serialized layout changes. Unknown newer data is dropped.
constexpr auto kSerializeVersion = qint32(2);
constexpr auto kOldestReadableVersion = qint32(1);
constexpr auto kHiddenSendersSinceVersion = qint32(2);

using IdsSet = base::flat_set<FullMsgId>;
using SendersSet = base::flat_set<std::pair<PeerId, PeerId>>;

[[nodiscard]] QString MembershipSettingKey() {
	return u"hide_membership_messages"_q;
}

void WriteSet(QDataStream &stream, const IdsSet &ids) {
	stream << quint32(ids.size());
	for (const auto &id : ids) {
		stream << SerializePeerId(id.peer) << qint64(id.msg.bare);
	}
}

[[nodiscard]] IdsSet ReadSet(QDataStream &stream) {
	auto result = IdsSet();
	auto count = quint32();
	stream >> count;
	for (auto i = quint32(); i != count; ++i) {
		auto peer = quint64();
		auto msg = qint64();
		stream >> peer >> msg;
		if (stream.status() != QDataStream::Ok) {
			break;
		}
		result.emplace(FullMsgId(DeserializePeerId(peer), MsgId(msg)));
	}
	return result;
}

void WriteSenders(QDataStream &stream, const SendersSet &senders) {
	stream << quint32(senders.size());
	for (const auto &[chat, sender] : senders) {
		stream << SerializePeerId(chat) << SerializePeerId(sender);
	}
}

[[nodiscard]] SendersSet ReadSenders(QDataStream &stream) {
	auto result = SendersSet();
	auto count = quint32();
	stream >> count;
	for (auto i = quint32(); i != count; ++i) {
		auto chat = quint64();
		auto sender = quint64();
		stream >> chat >> sender;
		if (stream.status() != QDataStream::Ok) {
			break;
		}
		result.emplace(DeserializePeerId(chat), DeserializePeerId(sender));
	}
	return result;
}

} // namespace

LocalStore::LocalStore(not_null<Main::Session*> session)
: _session(session) {
	load();

	FASettings::JsonSettings::Events(
		MembershipSettingKey()
	) | rpl::start_with_next([=](const QString &) {
		_session->data().requestItemViewsRefresh([](
				not_null<const HistoryItem*> item) {
			return item->Has<HistoryServiceMembership>();
		});
	}, _lifetime);

	for (const auto &key : FAMediaBlur::KindSettingKeys()) {
		FASettings::JsonSettings::Events(
			key
		) | rpl::start_with_next([=](const QString &) {
			refreshAllMedia();
		}, _lifetime);
	}
	// Strength is read while painting and changes on every slider step,
	// so a repaint is enough and a full refresh would be too heavy.
	FASettings::JsonSettings::Events(
		FAMediaBlur::StrengthSettingKey()
	) | rpl::start_with_next([=](const QString &) {
		_session->data().requestItemViewsRepaint(
			FAMediaBlur::HasBlurrableMedia);
	}, _lifetime);
}

void LocalStore::refreshAllMedia() const {
	const auto owner = &_session->data();
	owner->requestItemViewsRefresh(FAMediaBlur::HasBlurrableMedia);
	owner->histories().enumerate([](not_null<History*> history) {
		// Chat list previews cache their thumbnails until invalidated.
		const auto item = history->chatListMessage();
		if (item && FAMediaBlur::HasBlurrableMedia(item)) {
			history->lastItemDialogsView().itemInvalidated(item);
			history->updateChatListEntry();
		}
	});
}

bool LocalStore::isHidden(not_null<const HistoryItem*> item) const {
	if (!_hidden.empty() && _hidden.contains(item->fullId())) {
		return true;
	} else if (_hiddenSenders.empty() || item->out()) {
		return false;
	}
	return isSenderHidden(item->history()->peer->id, item->from()->id);
}

int LocalStore::hiddenCount(PeerId peer) const {
	return int(ranges::count(_hidden, peer, &FullMsgId::peer));
}

int LocalStore::hiddenCount() const {
	return int(_hidden.size());
}

void LocalStore::hide(const MessageIdsList &ids) {
	auto added = std::vector<FullMsgId>();
	const auto add = [&](FullMsgId id) {
		if (IsServerMsgId(id.msg) && !_hidden.contains(id)) {
			_hidden.emplace(id);
			added.push_back(id);
		}
	};
	const auto owner = &_session->data();
	for (const auto &id : ids) {
		const auto item = owner->message(id);
		const auto group = item ? owner->groups().find(item) : nullptr;
		if (group) {
			for (const auto &part : group->items) {
				add(part->fullId());
			}
		} else {
			add(id);
		}
	}
	if (added.empty()) {
		return;
	}
	save();
	refreshViews(added);
	_changes.fire({});
}

void LocalStore::unhide(PeerId peer) {
	unhideWhere([=](FullMsgId id) { return id.peer == peer; });
}

void LocalStore::unhideAll() {
	unhideWhere([](FullMsgId) { return true; });
}

void LocalStore::unhideWhere(Fn<bool(FullMsgId)> condition) {
	auto removed = std::vector<FullMsgId>();
	for (const auto &id : _hidden) {
		if (condition(id)) {
			removed.push_back(id);
		}
	}
	if (removed.empty()) {
		return;
	}
	for (const auto &id : removed) {
		_hidden.remove(id);
	}
	save();
	refreshViews(removed);
	_changes.fire({});
}

bool LocalStore::isSenderHidden(PeerId chat, PeerId sender) const {
	return !_hiddenSenders.empty()
		&& _hiddenSenders.contains(SenderKey{ chat, sender });
}

int LocalStore::hiddenSendersCount(PeerId chat) const {
	return int(ranges::count(_hiddenSenders, chat, &SenderKey::first));
}

int LocalStore::hiddenSendersCount() const {
	return int(_hiddenSenders.size());
}

void LocalStore::hideSender(PeerId chat, PeerId sender) {
	if (!chat || !sender || isSenderHidden(chat, sender)) {
		return;
	}
	_hiddenSenders.emplace(chat, sender);
	save();
	refreshChatViews({ chat });
	_changes.fire({});
}

void LocalStore::unhideSenders(PeerId chat) {
	unhideSendersWhere([=](const SenderKey &key) {
		return key.first == chat;
	});
}

void LocalStore::unhideAllSenders() {
	unhideSendersWhere([](const SenderKey &) { return true; });
}

void LocalStore::unhideSendersWhere(Fn<bool(const SenderKey&)> condition) {
	auto chats = base::flat_set<PeerId>();
	auto removed = std::vector<SenderKey>();
	for (const auto &key : _hiddenSenders) {
		if (condition(key)) {
			removed.push_back(key);
			chats.emplace(key.first);
		}
	}
	if (removed.empty()) {
		return;
	}
	for (const auto &key : removed) {
		_hiddenSenders.remove(key);
	}
	save();
	refreshChatViews(std::move(chats));
	_changes.fire({});
}

bool LocalStore::isMediaBlurToggled(FullMsgId id) const {
	return !_mediaBlurToggled.empty() && _mediaBlurToggled.contains(id);
}

void LocalStore::toggleMediaBlur(FullMsgId id) {
	if (!id) {
		return;
	} else if (_mediaBlurToggled.contains(id)) {
		_mediaBlurToggled.remove(id);
	} else {
		_mediaBlurToggled.emplace(id);
	}
	save();
	repaintMedia(id);
	_changes.fire({});
}

rpl::producer<> LocalStore::changes() const {
	return _changes.events();
}

void LocalStore::load() {
	const auto bytes = _session->local().readFALocalData();
	if (bytes.isEmpty()) {
		return;
	}
	QDataStream stream(bytes);
	stream.setVersion(QDataStream::Qt_5_1);
	auto version = qint32();
	stream >> version;
	if (stream.status() != QDataStream::Ok
		|| version < kOldestReadableVersion
		|| version > kSerializeVersion) {
		return;
	}
	auto hidden = ReadSet(stream);
	auto toggled = ReadSet(stream);
	auto senders = (version >= kHiddenSendersSinceVersion)
		? ReadSenders(stream)
		: SendersSet();
	if (stream.status() != QDataStream::Ok) {
		return;
	}
	_hidden = std::move(hidden);
	_mediaBlurToggled = std::move(toggled);
	_hiddenSenders = std::move(senders);
}

void LocalStore::save() const {
	auto bytes = QByteArray();
	{
		QDataStream stream(&bytes, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << kSerializeVersion;
		WriteSet(stream, _hidden);
		WriteSet(stream, _mediaBlurToggled);
		WriteSenders(stream, _hiddenSenders);
	}
	_session->local().writeFALocalData(bytes);
}

void LocalStore::refreshViews(const std::vector<FullMsgId> &ids) const {
	const auto owner = &_session->data();
	for (const auto &id : ids) {
		if (const auto item = owner->message(id)) {
			owner->requestItemViewRefresh(item);
		}
	}
}

void LocalStore::refreshChatViews(base::flat_set<PeerId> chats) const {
	_session->data().requestItemViewsRefresh([=](
			not_null<const HistoryItem*> item) {
		return chats.contains(item->history()->peer->id);
	});
}

void LocalStore::repaintMedia(FullMsgId id) const {
	const auto owner = &_session->data();
	const auto item = owner->message(id);
	if (!item) {
		return;
	}
	const auto repaint = [&](not_null<HistoryItem*> item) {
		// Chat list previews cache their thumbnails until invalidated.
		item->history()->lastItemDialogsView().itemInvalidated(item);
		if (const auto topic = item->topic()) {
			topic->lastItemDialogsView().itemInvalidated(item);
		}
		if (const auto sublist = item->savedSublist()) {
			sublist->lastItemDialogsView().itemInvalidated(item);
		}
		owner->requestItemRepaint(item);
	};
	if (const auto group = owner->groups().find(item)) {
		for (const auto &part : group->items) {
			repaint(part);
		}
	} else {
		repaint(item);
	}
}

bool IsHiddenLocally(not_null<const HistoryItem*> item) {
	if (item->Has<HistoryServiceMembership>()
		&& FASettings::JsonSettings::GetBool(MembershipSettingKey())) {
		return true;
	}
	return item->history()->session().faLocalStore().isHidden(item);
}

} // namespace FAData
