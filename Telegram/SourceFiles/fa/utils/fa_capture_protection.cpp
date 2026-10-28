/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/utils/fa_capture_protection.h"

#include "fa/settings/fa_settings.h"
#include "core/application.h"
#include "window/notifications_manager.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/qevent.h>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>

// Available since Windows 10 2004, older SDK headers lack it.
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif // WDA_EXCLUDEFROMCAPTURE
#endif // Q_OS_WIN

namespace FACaptureProtection {
namespace {

[[nodiscard]] QString SettingKey() {
	return u"screen_capture_protection"_q;
}

#ifdef Q_OS_WIN
void SetDwmFlag(HWND hwnd, DWORD attribute, bool value) {
	const auto flag = BOOL(value ? TRUE : FALSE);
	DwmSetWindowAttribute(hwnd, attribute, &flag, sizeof(flag));
}
#endif // Q_OS_WIN

void Apply(not_null<QWindow*> window, bool enabled) {
#ifdef Q_OS_WIN
	// Display affinity exists only for top-level windows,
	// native children are captured as a part of their top-level.
	if (!window->handle() || window->parent()) {
		return;
	}
	const auto hwnd = reinterpret_cast<HWND>(window->winId());
	if (!enabled) {
		SetWindowDisplayAffinity(hwnd, WDA_NONE);
	} else if (!SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)) {
		// Before Windows 10 2004 the window is captured as a black area.
		if (!SetWindowDisplayAffinity(hwnd, WDA_MONITOR)) {
			LOG(("FA Capture Protection: "
				"SetWindowDisplayAffinity failed, error %1."
				).arg(GetLastError()));
		}
	}

	// Taskbar thumbnails, Alt+Tab and Peek are drawn by the shell from
	// the window content, not affected by the display affinity.
	// The main window draws its own static preview on top of this,
	// see Platform::MainWindow::setupPreviewPasscodeLock.
	SetDwmFlag(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, enabled);
	SetDwmFlag(hwnd, DWMWA_DISALLOW_PEEK, enabled);
#endif // Q_OS_WIN
}

void ApplyToAll(bool enabled) {
	for (const auto window : QGuiApplication::allWindows()) {
		Apply(window, enabled);
	}
}

class Protector final : public QObject {
public:
	explicit Protector(QObject *parent) : QObject(parent) {
		ApplyToAll(Active());

		// Subscribed before any window, so the windows that refine
		// the defaults from ApplyToAll() get the change after it.
		FASettings::JsonSettings::Events(
			SettingKey()
		) | rpl::start_with_next([](const QString &) {
			ApplyToAll(Active());
			Core::App().notifications().createManager();
		}, _lifetime);
	}

protected:
	bool eventFilter(QObject *object, QEvent *event) override {
		if (event->type() == QEvent::PlatformSurface
			&& object->isWindowType()
			&& static_cast<QPlatformSurfaceEvent*>(
				event)->surfaceEventType()
					== QPlatformSurfaceEvent::SurfaceCreated
			&& Active()) {
			Apply(static_cast<QWindow*>(object), true);
		}
		return QObject::eventFilter(object, event);
	}

private:
	rpl::lifetime _lifetime;

};

} // namespace

void Start() {
	if (!Supported()) {
		return;
	}
	static const auto protector = [] {
		const auto result = new Protector(qApp);
		qApp->installEventFilter(result);
		return result;
	}();
	Q_UNUSED(protector);
}

bool Supported() {
#ifdef Q_OS_WIN
	return true;
#else // Q_OS_WIN
	return false;
#endif // Q_OS_WIN
}

bool Active() {
	return Supported()
		&& FASettings::JsonSettings::GetBool(SettingKey());
}

rpl::producer<bool> ActiveValue() {
	return rpl::single(
		Active()
	) | rpl::then(
		FASettings::JsonSettings::Events(
			SettingKey()
		) | rpl::map([](const QString &) {
			return Active();
		})
	) | rpl::distinct_until_changed();
}

bool Toggle() {
	const auto enabled = !FASettings::JsonSettings::GetBool(SettingKey());
	FASettings::JsonSettings::Set(SettingKey(), enabled);
	FASettings::JsonSettings::Write();
	return Active();
}

} // namespace FACaptureProtection
