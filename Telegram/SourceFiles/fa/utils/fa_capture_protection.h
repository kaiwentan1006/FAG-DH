/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#pragma once

#include <rpl/producer.h>

namespace FACaptureProtection {

// Keeps every window of the app out of screenshots, screen recordings,
// screen sharing and remote desktop streams while the setting is on.
// The windows stay visible on the local monitor. Live taskbar / Alt+Tab
// previews are replaced by static ones and system drawn notifications
// are replaced by the app drawn ones, which are protected as well.
void Start();

[[nodiscard]] bool Supported();
[[nodiscard]] bool Active();
[[nodiscard]] rpl::producer<bool> ActiveValue();

// Flips and saves the setting, returns the new state.
bool Toggle();

} // namespace FACaptureProtection
