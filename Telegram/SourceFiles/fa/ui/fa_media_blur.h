/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#pragma once

class HistoryItem;
class Painter;

namespace HistoryView {
class Media;
} // namespace HistoryView

namespace Main {
class Session;
} // namespace Main

namespace FAMediaBlur {

enum class Kind {
	Photo,
	Video,
	Sticker,
	File,
};

// Only self-contained visual media is classified here. Containers like
// albums or link previews blur their visual parts individually, and
// unwrapped stickers are blurred by UnwrappedMedia around the sticker only.
[[nodiscard]] std::optional<Kind> KindOf(
	not_null<const HistoryView::Media*> media);

// Settings that choose which kinds of media are blurred.
[[nodiscard]] std::vector<QString> KindSettingKeys();
[[nodiscard]] QString StrengthSettingKey();

[[nodiscard]] bool IsBlurred(Kind kind, not_null<const HistoryItem*> item);
[[nodiscard]] bool IsBlurred(
	not_null<const HistoryView::Media*> media,
	not_null<const HistoryItem*> item);

[[nodiscard]] bool HasBlurrableMedia(not_null<const HistoryItem*> item);
[[nodiscard]] bool IsItemMediaBlurred(not_null<const HistoryItem*> item);

// The message whose media is under the mouse cursor, preferring the exact
// album part when the cursor is over one.
[[nodiscard]] std::optional<FullMsgId> HoveredMediaItem(
	not_null<Main::Session*> session);

void Paint(
	Painter &p,
	QRect area,
	bool blurred,
	Fn<void(Painter&)> paint);

// For media drawing its controls (time, share button) outside of its own
// rect: the part of `outer` around `area` is painted without the blur.
void PaintKeepingOutside(
	Painter &p,
	QRect area,
	QRect outer,
	bool blurred,
	Fn<void(Painter&)> paint);

// Same strength as Paint(), keeps the size and pixel ratio of the image.
[[nodiscard]] QImage BlurImage(QImage image);

} // namespace FAMediaBlur
