/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/utils/fa_global_hotkeys.h"

#include "fa/lang/fa_lang.h"
#include "fa/settings/fa_settings.h"
#include "fa/utils/fa_capture_protection.h"

#include "base/flat_map.h"
#include "base/invoke_queued.h"
#include "core/application.h"
#include "mainwindow.h"
#include "window/window_controller.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QKeySequence>

#ifdef Q_OS_WIN
#include <windows.h>
#endif // Q_OS_WIN

namespace FAGlobalHotkeys {
namespace {

struct Combination {
	Qt::Key key = Qt::Key(0);
	Qt::KeyboardModifiers modifiers;
};

[[nodiscard]] std::optional<Combination> Parse(const QString &sequence) {
	const auto trimmed = sequence.trimmed();
	if (trimmed.isEmpty()) {
		return std::nullopt;
	}
	const auto parsed = QKeySequence::fromString(
		trimmed,
		QKeySequence::PortableText);
	if (parsed.count() != 1) {
		return std::nullopt;
	}
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const auto combination = parsed[0];
	const auto result = Combination{
		.key = combination.key(),
		.modifiers = combination.keyboardModifiers(),
	};
#else // Qt >= 6.0.0
	const auto value = parsed[0];
	const auto result = Combination{
		.key = Qt::Key(value & ~Qt::KeyboardModifierMask),
		.modifiers = Qt::KeyboardModifiers(value & Qt::KeyboardModifierMask),
	};
#endif // Qt < 6.0.0
	if (!result.key || result.key == Qt::Key_unknown) {
		return std::nullopt;
	}
	return result;
}

#ifdef Q_OS_WIN

constexpr auto kWindowClass = L"FAgramGlobalHotkeys";

void ToggleWindow() {
	const auto window = Core::App().activePrimaryWindow();
	if (!window) {
		return;
	}
	const auto widget = window->widget();
	const auto shown = !widget->isHidden()
		&& !widget->isMinimized()
		&& (QGuiApplication::applicationState() == Qt::ApplicationActive);
	if (!shown) {
		widget->showFromTray();
		return;
	}
	Core::App().hideMediaView();
	// Without a tray icon there would be no way back but the shortcut.
	if (!widget->minimizeToTray()) {
		widget->setWindowState(widget->windowState() | Qt::WindowMinimized);
	}
}

void ToggleCaptureProtection() {
	const auto active = FACaptureProtection::Toggle();
	const auto window = Core::App().activePrimaryWindow();
	if (window && !window->widget()->isHidden()) {
		window->showToast(FAlang::Translate(active
			? u"fa_screen_capture_protection_on"_q
			: u"fa_screen_capture_protection_off"_q));
	}
}

enum HotkeyId {
	kToggleWindowId = 1,
	kToggleCaptureProtectionId,
};

[[nodiscard]] UINT VirtualKey(Qt::Key key) {
	if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
		return VK_F1 + UINT(key - Qt::Key_F1);
	}
	switch (key) {
	case Qt::Key_Space: return VK_SPACE;
	case Qt::Key_Return:
	case Qt::Key_Enter: return VK_RETURN;
	case Qt::Key_Tab: return VK_TAB;
	case Qt::Key_Escape: return VK_ESCAPE;
	case Qt::Key_Backspace: return VK_BACK;
	case Qt::Key_Insert: return VK_INSERT;
	case Qt::Key_Delete: return VK_DELETE;
	case Qt::Key_Home: return VK_HOME;
	case Qt::Key_End: return VK_END;
	case Qt::Key_PageUp: return VK_PRIOR;
	case Qt::Key_PageDown: return VK_NEXT;
	case Qt::Key_Left: return VK_LEFT;
	case Qt::Key_Right: return VK_RIGHT;
	case Qt::Key_Up: return VK_UP;
	case Qt::Key_Down: return VK_DOWN;
	case Qt::Key_Pause: return VK_PAUSE;
	case Qt::Key_Print: return VK_SNAPSHOT;
	default: break;
	}
	// Letters, digits and punctuation of the current keyboard layout.
	if (key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde) {
		const auto scanned = VkKeyScanW(wchar_t(key));
		if (scanned != -1) {
			return LOBYTE(scanned);
		}
	}
	return 0;
}

[[nodiscard]] UINT NativeModifiers(Qt::KeyboardModifiers modifiers) {
	auto result = UINT(MOD_NOREPEAT);
	if (modifiers & Qt::ShiftModifier) {
		result |= MOD_SHIFT;
	}
	if (modifiers & Qt::ControlModifier) {
		result |= MOD_CONTROL;
	}
	if (modifiers & Qt::AltModifier) {
		result |= MOD_ALT;
	}
	if (modifiers & Qt::MetaModifier) {
		result |= MOD_WIN;
	}
	return result;
}

class Registrar final {
public:
	Registrar();

	void bind(int id, const QString &settingKey, Fn<void()> action);

private:
	struct Entry {
		QString settingKey;
		Fn<void()> action;
		bool registered = false;
	};

	static LRESULT CALLBACK Procedure(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam);

	void update(int id);
	void fire(int id);

	HWND _hwnd = nullptr;
	base::flat_map<int, Entry> _entries;
	rpl::lifetime _lifetime;

};

Registrar::Registrar() {
	const auto instance = GetModuleHandleW(nullptr);
	auto windowClass = WNDCLASSEXW{ sizeof(WNDCLASSEXW) };
	windowClass.lpfnWndProc = Procedure;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = kWindowClass;
	if (!RegisterClassExW(&windowClass)
		&& GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		LOG(("FA Global Hotkeys: RegisterClassEx failed, error %1."
			).arg(GetLastError()));
		return;
	}
	// A message-only window gets the hotkeys from any message loop,
	// including the modal ones of menus and window moving.
	_hwnd = CreateWindowExW(
		0,
		kWindowClass,
		L"",
		0,
		0,
		0,
		0,
		0,
		HWND_MESSAGE,
		nullptr,
		instance,
		nullptr);
	if (!_hwnd) {
		LOG(("FA Global Hotkeys: CreateWindowEx failed, error %1."
			).arg(GetLastError()));
		return;
	}
	SetWindowLongPtrW(_hwnd, GWLP_USERDATA, LONG_PTR(this));
}

void Registrar::bind(int id, const QString &settingKey, Fn<void()> action) {
	_entries[id] = Entry{ .settingKey = settingKey, .action = action };
	update(id);
	FASettings::JsonSettings::Events(
		settingKey
	) | rpl::start_with_next([=](const QString &) {
		update(id);
	}, _lifetime);
}

void Registrar::update(int id) {
	const auto i = _entries.find(id);
	if (!_hwnd || i == end(_entries)) {
		return;
	}
	auto &entry = i->second;
	if (entry.registered) {
		UnregisterHotKey(_hwnd, id);
		entry.registered = false;
	}
	const auto sequence = FASettings::JsonSettings::GetString(
		entry.settingKey);
	const auto combination = Parse(sequence);
	const auto key = combination ? VirtualKey(combination->key) : 0;
	if (!key) {
		if (!sequence.trimmed().isEmpty()) {
			LOG(("FA Global Hotkeys: Can't use '%1' for %2."
				).arg(sequence, entry.settingKey));
		}
		return;
	}
	entry.registered = RegisterHotKey(
		_hwnd,
		id,
		NativeModifiers(combination->modifiers),
		key);
	if (!entry.registered) {
		// Most likely another app already owns the same combination.
		LOG(("FA Global Hotkeys: RegisterHotKey '%1' failed, error %2."
			).arg(sequence).arg(GetLastError()));
	}
}

void Registrar::fire(int id) {
	const auto i = _entries.find(id);
	if (i != end(_entries) && i->second.action) {
		i->second.action();
	}
}

LRESULT CALLBACK Registrar::Procedure(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam) {
	if (message == WM_HOTKEY) {
		const auto that = reinterpret_cast<Registrar*>(
			GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (that) {
			const auto id = int(wParam);
			// Leave the native callback before touching the windows.
			InvokeQueued(qApp, [=] { that->fire(id); });
		}
		return 0;
	}
	return DefWindowProcW(hwnd, message, wParam, lParam);
}

#endif // Q_OS_WIN

} // namespace

void Start() {
#ifdef Q_OS_WIN
	static const auto registrar = [] {
		const auto result = new Registrar();
		result->bind(
			kToggleWindowId,
			ToggleWindowSettingKey(),
			ToggleWindow);
		result->bind(
			kToggleCaptureProtectionId,
			ToggleCaptureProtectionSettingKey(),
			ToggleCaptureProtection);
		return result;
	}();
	Q_UNUSED(registrar);
#endif // Q_OS_WIN
}

bool Supported() {
#ifdef Q_OS_WIN
	return true;
#else // Q_OS_WIN
	return false;
#endif // Q_OS_WIN
}

QString ToggleWindowSettingKey() {
	return u"toggle_window_hotkey"_q;
}

QString ToggleCaptureProtectionSettingKey() {
	return u"toggle_screen_capture_protection_hotkey"_q;
}

bool IsValid(const QString &sequence) {
	const auto combination = Parse(sequence);
#ifdef Q_OS_WIN
	return combination && (VirtualKey(combination->key) != 0);
#else // Q_OS_WIN
	return combination.has_value();
#endif // Q_OS_WIN
}

QString ReadableText(const QString &sequence) {
	return Parse(sequence)
		? QKeySequence::fromString(
			sequence.trimmed(),
			QKeySequence::PortableText).toString(QKeySequence::NativeText)
		: FAlang::Translate(u"fa_global_shortcut_off"_q);
}

} // namespace FAGlobalHotkeys
