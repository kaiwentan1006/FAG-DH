/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/ui/fa_autoscroll.h"

#include "fa/settings/fa_settings.h"

#include "base/timer.h"
#include "history/admin_log/history_admin_log_inner.h"
#include "history/history_inner_widget.h"
#include "history/view/history_view_list_widget.h"
#include "ui/click_handler.h"
#include "ui/widgets/elastic_scroll.h"
#include "ui/widgets/scroll_area.h"

#include <QtCore/QPointer>
#include <QtGui/QCursor>
#include <QtGui/QGuiApplication>
#include <QtGui/qevent.h>
#include <QtWidgets/QApplication>

namespace FAAutoscroll {
namespace {

constexpr auto kTickInterval = crl::time(16);

// The speed setting is in hundredths of a pixel per tick
// for every pixel the cursor is away from the press point.
constexpr auto kSpeedDivider = 100.;

struct Target {
	QPointer<QWidget> widget;
	Fn<int()> top;
	Fn<int()> max;
	Fn<void(int)> scrollTo;

	// A plain click starts scrolling only where the middle button
	// has no other meaning, in other lists the click is passed on.
	bool clickStarts = false;
};

[[nodiscard]] bool IsMessageList(QWidget *widget) {
	return dynamic_cast<HistoryInner*>(widget)
		|| dynamic_cast<HistoryView::ListWidget*>(widget)
		|| dynamic_cast<AdminLog::InnerWidget*>(widget);
}

template <typename Scroll>
[[nodiscard]] Target MakeTarget(not_null<Scroll*> scroll, bool clickStarts) {
	const auto raw = scroll.get();
	return {
		.widget = raw,
		.top = [=] { return raw->scrollTop(); },
		.max = [=] { return raw->scrollTopMax(); },
		.scrollTo = [=](int top) { raw->scrollToY(top); },
		.clickStarts = clickStarts,
	};
}

[[nodiscard]] std::optional<Target> FindTarget(QWidget *widget) {
	auto messages = false;
	for (auto w = widget; w; w = w->parentWidget()) {
		messages = messages || IsMessageList(w);
		if (const auto area = dynamic_cast<Ui::ScrollArea*>(w)) {
			if (area->scrollTopMax() > 0) {
				return MakeTarget<Ui::ScrollArea>(area, messages);
			}
		} else if (const auto elastic = dynamic_cast<Ui::ElasticScroll*>(w)) {
			if (elastic->scrollTopMax() > 0) {
				return MakeTarget<Ui::ElasticScroll>(elastic, messages);
			}
		}
		if (w->isWindow()) {
			break;
		}
	}
	return std::nullopt;
}

class Scroller final : public QObject {
public:
	explicit Scroller(QObject *parent);

protected:
	bool eventFilter(QObject *object, QEvent *event) override;

private:
	enum class State {
		None,
		Pressed,
		Holding,
		Sticky,
	};

	struct Press {
		QPointer<QWidget> receiver;
		QPoint local;
		QPoint global;
		Qt::KeyboardModifiers modifiers;
	};

	bool handlePress(not_null<QWidget*> receiver, not_null<QMouseEvent*> e);
	bool handleRelease(not_null<QMouseEvent*> e);
	void startScrolling(State state);
	void stop();
	void replayPress();
	void tick();
	[[nodiscard]] bool outsideDeadZone() const;

	base::Timer _timer;
	State _state = State::None;
	std::optional<Target> _target;
	Press _press;
	double _remainder = 0.;
	Qt::MouseButtons _swallowReleases;
	bool _cursorOverridden = false;
	bool _replaying = false;

};

Scroller::Scroller(QObject *parent)
: QObject(parent)
, _timer([=] { tick(); }) {
}

bool Scroller::eventFilter(QObject *object, QEvent *event) {
	if (_replaying) {
		return QObject::eventFilter(object, event);
	}
	switch (event->type()) {
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
		if (object->isWidgetType()) {
			const auto e = static_cast<QMouseEvent*>(event);
			if (handlePress(static_cast<QWidget*>(object), e)) {
				return true;
			}
		}
		break;
	case QEvent::MouseButtonRelease:
		if (object->isWidgetType()
			&& handleRelease(static_cast<QMouseEvent*>(event))) {
			return true;
		}
		break;
	case QEvent::KeyPress:
		if (_state != State::None
			&& static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
			stop();
			return true;
		}
		break;
	case QEvent::MouseMove:
		// Lists treat a move with the middle button held as a selection.
		if (_state != State::None && object->isWidgetType()) {
			return true;
		}
		break;
	case QEvent::Wheel:
		if (_state == State::Sticky) {
			stop();
		}
		break;
	case QEvent::ApplicationStateChange:
		if (static_cast<QApplicationStateChangeEvent*>(
				event)->applicationState() != Qt::ApplicationActive) {
			stop();
		}
		break;
	default:
		break;
	}
	return QObject::eventFilter(object, event);
}

bool Scroller::handlePress(
		not_null<QWidget*> receiver,
		not_null<QMouseEvent*> e) {
	if (_state == State::Sticky) {
		// The click that ends sticky scrolling is not a click on the list.
		_swallowReleases.setFlag(e->button());
		stop();
		return true;
	} else if (_state != State::None) {
		return (e->button() == Qt::MiddleButton);
	} else if (e->button() != Qt::MiddleButton
		|| e->modifiers() != Qt::NoModifier
		|| ClickHandler::getActive()
		|| !FASettings::JsonSettings::GetBool(EnabledSettingKey())) {
		return false;
	}
	_target = FindTarget(receiver);
	if (!_target) {
		return false;
	}
	_press = Press{
		.receiver = receiver.get(),
		.local = e->pos(),
		.global = e->globalPos(),
		.modifiers = e->modifiers(),
	};
	_state = State::Pressed;
	_remainder = 0.;
	_timer.callEach(kTickInterval);
	return true;
}

bool Scroller::handleRelease(not_null<QMouseEvent*> e) {
	if (e->button() != Qt::NoButton
		&& _swallowReleases.testFlag(e->button())) {
		_swallowReleases.setFlag(e->button(), false);
		return true;
	} else if (e->button() != Qt::MiddleButton) {
		return false;
	}
	switch (_state) {
	case State::Pressed:
		if (outsideDeadZone()) {
			stop();
		} else if (_target && _target->clickStarts) {
			startScrolling(State::Sticky);
		} else {
			stop();
			replayPress();
		}
		return true;
	case State::Holding:
		stop();
		return true;
	case State::Sticky:
	case State::None:
		break;
	}
	return false;
}

void Scroller::startScrolling(State state) {
	_state = state;
	if (!_cursorOverridden) {
		_cursorOverridden = true;
		QGuiApplication::setOverrideCursor(Qt::SizeVerCursor);
	}
}

void Scroller::stop() {
	if (_state == State::None) {
		return;
	}
	_state = State::None;
	_target = std::nullopt;
	_timer.cancel();
	if (_cursorOverridden) {
		_cursorOverridden = false;
		QGuiApplication::restoreOverrideCursor();
	}
}

void Scroller::replayPress() {
	const auto receiver = _press.receiver.data();
	if (!receiver) {
		return;
	}
	auto press = QMouseEvent(
		QEvent::MouseButtonPress,
		_press.local,
		_press.global,
		Qt::MiddleButton,
		Qt::MiddleButton,
		_press.modifiers);
	auto release = QMouseEvent(
		QEvent::MouseButtonRelease,
		_press.local,
		_press.global,
		Qt::MiddleButton,
		Qt::NoButton,
		_press.modifiers);
	_replaying = true;
	QApplication::sendEvent(receiver, &press);
	if (const auto still = _press.receiver.data()) {
		QApplication::sendEvent(still, &release);
	}
	_replaying = false;
}

bool Scroller::outsideDeadZone() const {
	const auto dead = FASettings::JsonSettings::GetInt(DeadZoneSettingKey());
	return std::abs(QCursor::pos().y() - _press.global.y()) > dead;
}

void Scroller::tick() {
	if (!_target || !_target->widget) {
		stop();
		return;
	} else if (_state == State::Pressed) {
		if (!(QGuiApplication::mouseButtons() & Qt::MiddleButton)) {
			return;
		} else if (!outsideDeadZone()) {
			return;
		}
		startScrolling(State::Holding);
	}
	const auto offset = QCursor::pos().y() - _press.global.y();
	const auto dead = FASettings::JsonSettings::GetInt(DeadZoneSettingKey());
	const auto distance = std::abs(offset) - dead;
	if (distance <= 0) {
		_remainder = 0.;
		return;
	}
	const auto speed = FASettings::JsonSettings::GetInt(SpeedSettingKey());
	_remainder += ((offset > 0) ? 1 : -1) * distance * speed / kSpeedDivider;
	const auto delta = int(_remainder);
	if (!delta) {
		return;
	}
	_remainder -= delta;
	const auto top = _target->top();
	const auto updated = std::clamp(top + delta, 0, _target->max());
	if (updated != top) {
		_target->scrollTo(updated);
	} else {
		_remainder = 0.;
	}
}

} // namespace

void Start() {
	static const auto scroller = [] {
		const auto result = new Scroller(qApp);
		qApp->installEventFilter(result);
		return result;
	}();
	Q_UNUSED(scroller);
}

QString EnabledSettingKey() {
	return u"middle_click_autoscroll"_q;
}

QString SpeedSettingKey() {
	return u"middle_click_autoscroll_speed"_q;
}

QString DeadZoneSettingKey() {
	return u"middle_click_autoscroll_dead_zone"_q;
}

} // namespace FAAutoscroll
