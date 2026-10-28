/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#pragma once

namespace FAGlobalHotkeys {

// System wide shortcuts, they work while the app is hidden or inactive.
// Each one is a key sequence string in the settings, empty turns it off.
// While registered, the system delivers the combination only to the app.
void Start();

[[nodiscard]] bool Supported();

[[nodiscard]] QString ToggleWindowSettingKey();
[[nodiscard]] QString ToggleCaptureProtectionSettingKey();

[[nodiscard]] bool IsValid(const QString &sequence);
[[nodiscard]] QString ReadableText(const QString &sequence);

} // namespace FAGlobalHotkeys
