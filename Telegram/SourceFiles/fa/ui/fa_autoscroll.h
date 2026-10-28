/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#pragma once

namespace FAAutoscroll {

// Middle button autoscroll for every scrollable list of the app.
//
// Pressing the middle button and moving the mouse scrolls the list under
// the cursor while the button is held, the speed grows with the distance
// from the press point. A plain middle click in a message list keeps
// scrolling until the next click, Escape or a wheel turn. Middle clicks
// on links and plain middle clicks in other lists behave as before.
void Start();

[[nodiscard]] QString EnabledSettingKey();
[[nodiscard]] QString SpeedSettingKey();
[[nodiscard]] QString DeadZoneSettingKey();

} // namespace FAAutoscroll
