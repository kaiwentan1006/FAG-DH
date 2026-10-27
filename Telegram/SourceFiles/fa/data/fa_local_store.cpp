/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/data/fa_local_store.h"

#include "data/data_groups.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "storage/storage_account.h"

#include <QtCore/QDataStream>

namespace FAData {
namespace {

// Bump when the serialized layout changes, older data is then dropped.
constexpr auto kSerializeVersion = qint32(1);

using IdsSet = base::flat_set<FullMsgId>;

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

} // namespace

LocalStore::LocalStore(not_null<Main::Session*> session)
: _session(session) {
	load();
}

bool LocalStore::isHidden(FullMsgId id) const {
	return !_hidden.empty() && _hidden.contains(id);
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
		|| version != kSerializeVersion) {
		return;
	}
	auto hidden = ReadSet(stream);
	auto toggled = ReadSet(stream);
	if (stream.status() != QDataStream::Ok) {
		return;
	}
	_hidden = std::move(hidden);
	_mediaBlurToggled = std::move(toggled);
}

void LocalStore::save() const {
	auto bytes = QByteArray();
	{
		QDataStream stream(&bytes, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << kSerializeVersion;
		WriteSet(stream, _hidden);
		WriteSet(stream, _mediaBlurToggled);
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

void LocalStore::repaintMedia(FullMsgId id) const {
	const auto owner = &_session->data();
	const auto item = owner->message(id);
	if (!item) {
		return;
	} else if (const auto group = owner->groups().find(item)) {
		for (const auto &part : group->items) {
			owner->requestItemRepaint(part);
		}
	} else {
		owner->requestItemRepaint(item);
	}
}

bool IsHiddenLocally(not_null<const HistoryItem*> item) {
	return item->history()->session().faLocalStore().isHidden(
		item->fullId());
}

} // namespace FAData
