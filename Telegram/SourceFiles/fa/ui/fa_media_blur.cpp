/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fajox1/fagramdesktop/blob/master/LEGAL
*/

#include "fa/ui/fa_media_blur.h"

#include "fa/data/fa_local_store.h"
#include "fa/settings/fa_settings.h"

#include "data/data_document.h"
#include "data/data_file_click_handler.h"
#include "data/data_media_types.h"
#include "data/data_session.h"
#include "data/data_web_page.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "history/view/media/history_view_document.h"
#include "history/view/media/history_view_gif.h"
#include "history/view/media/history_view_photo.h"
#include "main/main_session.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"

namespace FAMediaBlur {
namespace {

[[nodiscard]] QString SettingKey(Kind kind) {
	switch (kind) {
	case Kind::Photo: return u"blur_photos"_q;
	case Kind::Video: return u"blur_videos"_q;
	case Kind::Sticker: return u"blur_stickers"_q;
	case Kind::File: return u"blur_files"_q;
	}
	Unexpected("Kind in FAMediaBlur::SettingKey.");
}

[[nodiscard]] std::optional<Kind> DocumentKind(
		not_null<const DocumentData*> document) {
	if (document->sticker()) {
		return Kind::Sticker;
	} else if (document->isAnimation()
		|| document->isVideoFile()
		|| document->isVideoMessage()) {
		return Kind::Video;
	} else if (document->hasThumbnail()) {
		return Kind::File;
	}
	return std::nullopt;
}

[[nodiscard]] std::optional<Kind> ItemKind(
		not_null<const HistoryItem*> item) {
	const auto media = item->media();
	if (!media) {
		return std::nullopt;
	} else if (const auto webpage = media->webpage()) {
		if (webpage->document) {
			return DocumentKind(webpage->document);
		} else if (webpage->photo) {
			return Kind::Photo;
		}
		return std::nullopt;
	} else if (media->photo()) {
		return Kind::Photo;
	} else if (const auto document = media->document()) {
		return DocumentKind(document);
	}
	return std::nullopt;
}

} // namespace

std::optional<Kind> KindOf(not_null<const HistoryView::Media*> media) {
	const auto raw = media.get();
	if (dynamic_cast<const HistoryView::Photo*>(raw)) {
		return Kind::Photo;
	} else if (dynamic_cast<const HistoryView::Gif*>(raw)) {
		return Kind::Video;
	} else if (dynamic_cast<const HistoryView::Document*>(raw)) {
		const auto document = media->getDocument();
		if (document && document->hasThumbnail()) {
			return Kind::File;
		}
	}
	return std::nullopt;
}

bool IsBlurred(Kind kind, not_null<const HistoryItem*> item) {
	const auto byDefault = FASettings::JsonSettings::GetBool(
		SettingKey(kind));
	const auto toggled = item->history()->session().faLocalStore(
	).isMediaBlurToggled(item->fullId());
	return (byDefault != toggled);
}

bool IsBlurred(
		not_null<const HistoryView::Media*> media,
		not_null<const HistoryItem*> item) {
	const auto kind = KindOf(media);
	return kind && IsBlurred(*kind, item);
}

bool HasBlurrableMedia(not_null<const HistoryItem*> item) {
	return ItemKind(item).has_value();
}

bool IsItemMediaBlurred(not_null<const HistoryItem*> item) {
	const auto kind = ItemKind(item);
	return kind && IsBlurred(*kind, item);
}

std::optional<FullMsgId> HoveredMediaItem(
		not_null<Main::Session*> session) {
	if (const auto link = ClickHandler::getActive()) {
		if (const auto file = dynamic_cast<FileClickHandler*>(link.get())) {
			const auto item = session->data().message(file->context());
			if (item && HasBlurrableMedia(item)) {
				return item->fullId();
			}
		}
	}
	using HistoryView::Element;
	const auto views = {
		Element::HoveredLink(),
		Element::Hovered(),
		Element::Moused(),
	};
	for (const auto view : views) {
		if (view
			&& (&view->history()->session() == session)
			&& HasBlurrableMedia(view->data())) {
			return view->data()->fullId();
		}
	}
	return std::nullopt;
}

void Paint(
		Painter &p,
		QRect area,
		bool blurred,
		Fn<void(Painter&)> paint) {
	if (!blurred || area.isEmpty()) {
		paint(p);
		return;
	}
	const auto downscale = FASettings::JsonSettings::GetInt(
		u"blur_strength"_q);
	const auto full = area.size() * style::DevicePixelRatio();
	const auto small = QSize(
		std::max(full.width() / downscale, 1),
		std::max(full.height() / downscale, 1));
	auto image = QImage(small, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	{
		auto q = Painter(&image);
		q.setRenderHint(QPainter::SmoothPixmapTransform);
		q.scale(
			small.width() / float64(area.width()),
			small.height() / float64(area.height()));
		q.translate(-area.topLeft());
		paint(q);
	}
	image = Images::Blur(std::move(image));

	auto hq = PainterHighQualityEnabler(p);
	p.drawImage(area, image);
}

} // namespace FAMediaBlur
