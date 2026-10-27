/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

// thanks rabbitGram

#pragma once

#include <rpl/producer.h>

#include <QtCore/QVariant>
#include <QtCore/QJsonArray>

#include <optional>

namespace FASettings {

namespace JsonSettings {

void Start();
void Load();
void Write();
void Finish();

[[nodiscard]] QVariant Get(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false);
[[nodiscard]] QVariant GetWithPending(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false);
[[nodiscard]] rpl::producer<QString> Events(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false);
[[nodiscard]] rpl::producer<QString> EventsWithPending(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false);
void Set(
	const QString &key,
	QVariant value,
	uint64 accountId = 0,
	bool isTestAccount = false);
void SetAfterRestart(
	const QString &key,
	QVariant value,
	uint64 accountId = 0,
	bool isTestAccount = false);
void Reset(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false);
void ResetAfterRestart(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false);

inline bool GetBool(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return Get(key, accountId, isTestAccount).toBool();
}

inline int GetInt(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return Get(key, accountId, isTestAccount).toInt();
}

inline QString GetString(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return Get(key, accountId, isTestAccount).toString();
}

inline QJsonArray GetJsonArray(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return Get(key, accountId, isTestAccount).toJsonArray();
}

inline bool GetBoolWithPending(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return GetWithPending(key, accountId, isTestAccount).toBool();
}

inline int GetIntWithPending(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return GetWithPending(key, accountId, isTestAccount).toInt();
}

inline QString GetStringWithPending(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return GetWithPending(key, accountId, isTestAccount).toString();
}

inline QJsonArray GetJsonArrayWithPending(
	const QString &key,
	uint64 accountId = 0,
	bool isTestAccount = false) {
	return GetWithPending(key, accountId, isTestAccount).toJsonArray();
}

struct IntRange {
	int min = 0;
	int max = 0;
};

// Allowed bounds of an integer setting, std::nullopt if unbounded.
[[nodiscard]] std::optional<IntRange> IntSettingRange(const QString &key);

// Corner radius as a fraction of the userpic size,
// or std::nullopt when the upstream userpic shapes should be used.
[[nodiscard]] std::optional<float64> UserpicRoundness();

} // namespace JsonSettings
} // namespace FASettings