/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/lang/fa_lang.h"

#include "lang/lang_instance.h"

namespace FAlang {
namespace {

const auto kDefaultLanguage = qsl("en");

QString LangCode = kDefaultLanguage;
QHash<QString, QString> Current;
QHash<QString, QString> Fallback;
bool FallbackLoaded = false;

[[nodiscard]] QHash<QString, QString> ReadLangFile(const QString &code) {
	auto result = QHash<QString, QString>();
	auto file = QFile(qsl(":/fa_lang/%1.json").arg(code));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return result;
	}
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		return result;
	}
	const auto object = document.object();
	result.reserve(object.size());
	for (auto i = object.constBegin(); i != object.constEnd(); ++i) {
		const auto value = i.value().toString();
		if (!value.isEmpty()) {
			result.insert(i.key(), value);
		}
	}
	return result;
}

void EnsureFallbackLoaded() {
	if (!FallbackLoaded) {
		FallbackLoaded = true;
		Fallback = ReadLangFile(kDefaultLanguage);
	}
}

} // namespace

rpl::producer<QString> RplTranslate(const QString &key) {
	return rpl::single(rpl::empty) | rpl::then(
		Lang::GetInstance().updated()
	) | rpl::map([=] {
		return Translate(key);
	});
}

QString Translate(const QString &key) {
	const auto i = Current.constFind(key);
	if (i != Current.constEnd()) {
		return *i;
	}
	EnsureFallbackLoaded();
	const auto j = Fallback.constFind(key);
	return (j != Fallback.constEnd()) ? *j : key;
}

void Load(const QString &baseLangCode, const QString &langCode) {
	auto code = langCode.isEmpty() ? baseLangCode : langCode;
	code.replace(qsl("-raw"), QString());
	if (code.isEmpty()) {
		code = kDefaultLanguage;
	}
	if (code == LangCode && (!Current.isEmpty() || code == kDefaultLanguage)) {
		return;
	}
	LangCode = code;
	Current = (code == kDefaultLanguage)
		? QHash<QString, QString>()
		: ReadLangFile(code);
}

} // namespace FAlang
