/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#pragma once

namespace Ui {
class VerticalLayout;
} // namespace Ui

namespace Window {
class SessionController;
} // namespace Window

namespace FASettings {

void AddJsonToggle(
	not_null<Ui::VerticalLayout*> container,
	const QString &langKey,
	const QString &option);

void AddJsonRestartToggle(
	not_null<Ui::VerticalLayout*> container,
	not_null<Window::SessionController*> controller,
	const QString &langKey,
	const QString &option);

// Slider over the IntRange declared for the option in the definition map.
void AddJsonSlider(
	not_null<Ui::VerticalLayout*> container,
	const QString &option,
	Fn<QString(int)> label,
	Fn<void(int)> changed = nullptr);

} // namespace FASettings
