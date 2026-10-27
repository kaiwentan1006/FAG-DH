/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/utils/fa_capture_protection.h"

#include "fa/settings/fa_settings.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/qevent.h>

#ifdef Q_OS_WIN
#include <windows.h>

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

[[nodiscard]] bool Enabled() {
	return FASettings::JsonSettings::GetBool(SettingKey());
}

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
		SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
	}
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
		ApplyToAll(Enabled());
		FASettings::JsonSettings::Events(
			SettingKey()
		) | rpl::start_with_next([](const QString &) {
			ApplyToAll(Enabled());
		}, _lifetime);
	}

protected:
	bool eventFilter(QObject *object, QEvent *event) override {
		if (event->type() == QEvent::PlatformSurface
			&& object->isWindowType()
			&& static_cast<QPlatformSurfaceEvent*>(
				event)->surfaceEventType()
					== QPlatformSurfaceEvent::SurfaceCreated
			&& Enabled()) {
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

} // namespace FACaptureProtection
