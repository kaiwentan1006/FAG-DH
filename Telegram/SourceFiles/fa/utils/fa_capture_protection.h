/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#pragma once

namespace FACaptureProtection {

// Keeps every window of the app out of screenshots, screen recordings,
// screen sharing and remote desktop streams while the setting is on.
// The windows stay visible on the local monitor.
void Start();

[[nodiscard]] bool Supported();

} // namespace FACaptureProtection
