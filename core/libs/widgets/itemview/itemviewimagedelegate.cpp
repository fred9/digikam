/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-04-19
 * Description : Qt item view for images - the delegate
 *
 * Copyright (C) 2002-2005 by Renchi Raju <renchi dot raju at gmail dot com>
 * Copyright (C) 2009      by Andi Clemens <andi dot clemens at gmail dot com>
 * Copyright (C) 2006-2011 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
 * Copyright (C) 2002-2018 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "itemviewimagedelegate.h"
#include "itemviewimagedelegatepriv.h"

// C++ includes

#include <cmath>

// Qt includes

#include <QCache>
#include <QPainter>
#include <QIcon>
#include <QApplication>

// KDE includes

#include <klocalizedstring.h>

// Local includes

#include "digikam_debug.h"
#include "imagepropertiestab.h"
#include "itemdelegateoverlay.h"
#include "thememanager.h"
#include "colorlabelwidget.h"
#include "ratingwidget.h"

namespace Digikam
{

ItemViewImageDelegatePrivate::ItemViewImageDelegatePrivate()
{
    q         = 0;
    spacing   = 0;
    thumbSize = ThumbnailSize(0);

    // painting constants
    radius    = 3;
    margin    = 5;

    makeStarPolygon();

    ratingPixmaps = QVector<QPixmap>(10);
}

void ItemViewImageDelegatePrivate::init(ItemViewImageDelegate* const _q)
{
    q = _q;

    q->connect(ThemeManager::instance(), SIGNAL(signalThemeChanged()),
               q, SLOT(slotThemeChanged()));
}

void ItemViewImageDelegatePrivate::clearRects()
{
    gridSize   = QSize(0, 0);
    rect       = QRect(0, 0, 0, 0);
    ratingRect = QRect(0, 0, 0, 0);
}

void ItemViewImageDelegatePrivate::makeStarPolygon()
{
    // Pre-computed star polygon for a 15x15 pixmap.
    starPolygon     = RatingWidget::starPolygon();
    starPolygonSize = QSize(15, 15);
}

ItemViewImageDelegate::ItemViewImageDelegate(QObject* const parent)
    : DItemDelegate(parent),
      d_ptr(new ItemViewImageDelegatePrivate)
{
    d_ptr->init(this);
}

ItemViewImageDelegate::ItemViewImageDelegate(ItemViewImageDelegatePrivate& dd, QObject* const parent)
    : DItemDelegate(parent),
      d_ptr(&dd)
{
    d_ptr->init(this);
}

ItemViewImageDelegate::~ItemViewImageDelegate()
{
    Q_D(ItemViewImageDelegate);

    removeAllOverlays();
    delete d;
}

ThumbnailSize ItemViewImageDelegate::thumbnailSize() const
{
    Q_D(const ItemViewImageDelegate);

    return d->thumbSize;
}

void ItemViewImageDelegate::setThumbnailSize(const ThumbnailSize& thumbSize)
{
    Q_D(ItemViewImageDelegate);

    if (d->thumbSize != thumbSize)
    {
        d->thumbSize = thumbSize;
        invalidatePaintingCache();
    }
}

void ItemViewImageDelegate::setSpacing(int spacing)
{
    Q_D(ItemViewImageDelegate);

    if (d->spacing == spacing)
    {
        return;
    }

    d->spacing = spacing;
    invalidatePaintingCache();
}

int ItemViewImageDelegate::spacing() const
{
    Q_D(const ItemViewImageDelegate);

    return d->spacing;
}

QRect ItemViewImageDelegate::rect() const
{
    Q_D(const ItemViewImageDelegate);

    return d->rect;
}

QRect ItemViewImageDelegate::pixmapRect() const
{
    return QRect();
}

QRect ItemViewImageDelegate::imageInformationRect() const
{
    return QRect();
}

QRect ItemViewImageDelegate::ratingRect() const
{
    Q_D(const ItemViewImageDelegate);

    return d->ratingRect;
}

void ItemViewImageDelegate::setRatingEdited(const QModelIndex& index)
{
    Q_D(ItemViewImageDelegate);

    d->editingRating = index;
}

QSize ItemViewImageDelegate::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    Q_D(const ItemViewImageDelegate);

    return d->rect.size();
}

QSize ItemViewImageDelegate::gridSize() const
{
    Q_D(const ItemViewImageDelegate);

    return d->gridSize;
}

bool ItemViewImageDelegate::acceptsToolTip(const QPoint&, const QRect& visualRect, const QModelIndex&, QRect* retRect) const
{
    if (retRect)
    {
        *retRect = visualRect;
    }

    return true;
}

bool ItemViewImageDelegate::acceptsActivation(const QPoint& , const QRect& visualRect, const QModelIndex&, QRect* retRect) const
{
    if (retRect)
    {
        *retRect = visualRect;
    }

    return true;
}

QAbstractItemDelegate* ItemViewImageDelegate::asDelegate()
{
    return this;
}

void ItemViewImageDelegate::overlayDestroyed(QObject* o)
{
    ItemDelegateOverlayContainer::overlayDestroyed(o);
}

void ItemViewImageDelegate::mouseMoved(QMouseEvent* e, const QRect& visualRect, const QModelIndex& index)
{
    // 3-way indirection DItemDelegate -> ItemViewImageDelegate -> ItemDelegateOverlayContainer
    ItemDelegateOverlayContainer::mouseMoved(e, visualRect, index);
}

void ItemViewImageDelegate::setDefaultViewOptions(const QStyleOptionViewItem& option)
{
    Q_D(ItemViewImageDelegate);

    d->font = option.font;
    invalidatePaintingCache();
}

void ItemViewImageDelegate::slotThemeChanged()
{
    invalidatePaintingCache();
}

void ItemViewImageDelegate::slotSetupChanged()
{
    invalidatePaintingCache();
}

void ItemViewImageDelegate::invalidatePaintingCache()
{
    Q_D(ItemViewImageDelegate);

    QSize oldGridSize = d->gridSize;
    updateSizeRectsAndPixmaps();

    if (oldGridSize != d->gridSize)
    {
        emit gridSizeChanged(d->gridSize);
        //emit sizeHintChanged(QModelIndex());
    }

    emit visualChange();
}

QRect ItemViewImageDelegate::drawThumbnail(QPainter* p, const QRect& thumbRect, const QPixmap& background,
                                           const QPixmap& thumbnail, bool isGrouped) const
{
    Q_D(const ItemViewImageDelegate);

    p->drawPixmap(0, 0, background);

    if (thumbnail.isNull())
    {
        return QRect();
    }

    QRect r = thumbRect;
/*
    p->drawPixmap(r.x() + (r.width()-thumbnail.width())/2,
                  r.y() + (r.height()-thumbnail.height())/2,
                  thumbnail);
*/

    QRect actualPixmapRect(r.x() + (r.width()-thumbnail.width())/2,
                           r.y() + (r.height()-thumbnail.height())/2,
                           thumbnail.width(), thumbnail.height());
/*
    p->save();
    QRegion pixmapClipRegion = QRegion(d->rect) - QRegion(actualPixmapRect);
    p->setClipRegion(pixmapClipRegion);

    p->drawPixmap(0, 0, background);
*/
    QPixmap borderPix = thumbnailBorderPixmap(actualPixmapRect.size(), isGrouped);

    if (isGrouped)
    {
        const int xPadding = (borderPix.width()-actualPixmapRect.width())/2;
        const int yPadding = (borderPix.height()-actualPixmapRect.height())/2;

        p->drawPixmap(actualPixmapRect.x()-xPadding,
                      actualPixmapRect.y()-yPadding, borderPix);

        QPixmap groupThumbnail = thumbnail.scaled(thumbnail.width()-10,
                                                  thumbnail.height()-10,
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation);

        p->drawPixmap(r.x() + (r.width()-groupThumbnail.width())/2,
                      r.y() + (r.height()-groupThumbnail.height())/2,
                      groupThumbnail);
    }
    else
    {
        p->drawPixmap(actualPixmapRect.x()-d->radius,
                      actualPixmapRect.y()-d->radius, borderPix);

        p->drawPixmap(r.x() + (r.width()-thumbnail.width())/2,
                      r.y() + (r.height()-thumbnail.height())/2,
                      thumbnail);
    }

    //p->restore();
    return actualPixmapRect;
}

void ItemViewImageDelegate::drawRating(QPainter* p, const QModelIndex& index, const QRect& ratingRect,
                                       int rating, bool isSelected) const
{
    Q_D(const ItemViewImageDelegate);

    if (d->editingRating != index)
    {
        p->drawPixmap(ratingRect, ratingPixmap(rating, isSelected));
    }
}

void ItemViewImageDelegate::drawSpecialInfo(QPainter* p,const QRect& r, const QString& text) const
{
    Q_D(const ItemViewImageDelegate);

    if (!text.isEmpty() && !r.isNull())
    {
        p->save();

        QFont fnt(d->fontReg);
        fnt.setWeight(QFont::Black);
        fnt.setItalic(false);
        p->setFont(fnt);
        p->setPen(QPen(Qt::gray));
        p->setOpacity(0.50);

        int flags   = Qt::AlignCenter | Qt::TextWordWrap;
        QRect bRect = p->boundingRect(r, flags, text);

        if (bRect.width() > r.width())
        {
            flags = Qt::AlignCenter | Qt::TextWrapAnywhere;
            bRect = p->boundingRect(r, flags, text);
        }

        p->fillRect(bRect, Qt::SolidPattern);
        p->setPen(QPen(Qt::white));
        p->setOpacity(1.0);
        p->drawText(bRect, flags, text);

        p->restore();
    }
}

void ItemViewImageDelegate::drawName(QPainter* p,const QRect& nameRect, const QString& name) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontReg);
    // NOTE: in case of file name are long, use squeezedTextCached to adjust string elide mode.
    // See bug #278664 for details
    p->drawText(nameRect, Qt::AlignCenter, squeezedTextCached(p, nameRect.width(), name));
}

void ItemViewImageDelegate::drawTitle(QPainter* p, const QRect& titleRect, const QString& title) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontReg);
    p->drawText(titleRect, Qt::AlignCenter, squeezedTextCached(p, titleRect.width(), title));
}

void ItemViewImageDelegate::drawComments(QPainter* p, const QRect& commentsRect, const QString& comments) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontCom);
    p->drawText(commentsRect, Qt::AlignCenter, squeezedTextCached(p, commentsRect.width(), comments));
}

void ItemViewImageDelegate::drawCreationDate(QPainter* p, const QRect& dateRect, const QDateTime& date) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontXtra);
    QString str = dateToString(date);
    //str         = i18nc("date of image creation", "created: %1", str);
    p->drawText(dateRect, Qt::AlignCenter, str); //squeezedTextCached(p, dateRect.width(), str));
}

void ItemViewImageDelegate::drawModificationDate(QPainter* p, const QRect& dateRect, const QDateTime& date) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontXtra);
    QString str = dateToString(date);
    str         = i18nc("date of last image modification", "Mod.: %1",str);
    p->drawText(dateRect, Qt::AlignCenter, str); //squeezedTextCached(p, dateRect.width(), str));
}

void ItemViewImageDelegate::drawImageSize(QPainter* p, const QRect& dimsRect, const QSize& dims) const
{
    Q_D(const ItemViewImageDelegate);

    if (dims.isValid())
    {
        p->setFont(d->fontXtra);
        QString mpixels, resolution;
        mpixels.setNum(dims.width()*dims.height()/1000000.0, 'f', 2);

        if (dims.isValid())
            resolution = i18nc("%1 width, %2 height, %3 mpixels", "%1x%2 (%3Mpx)",
                               dims.width(), dims.height(), mpixels);
        else
        {
            resolution = i18nc("unknown image resolution", "Unknown");
        }

        p->drawText(dimsRect, Qt::AlignCenter, resolution); //squeezedTextCached(p, dimsRect.width(), resolution));
    }
}

void ItemViewImageDelegate::drawAspectRatio(QPainter* p, const QRect& dimsRect, const QSize& dims) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontXtra);
    QString resolution;

    if (dims.isValid())
    {
        ImagePropertiesTab::aspectRatioToString(dims.width(), dims.height(), resolution);
    }
    else
    {
        resolution = i18nc("unknown image resolution", "Unknown");
    }

    p->drawText(dimsRect, Qt::AlignCenter, resolution); //squeezedTextCached(p, dimsRect.width(), resolution));
}

void ItemViewImageDelegate::drawFileSize(QPainter* p, const QRect& r, qlonglong bytes) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontXtra);
    p->drawText(r, Qt::AlignCenter, ImagePropertiesTab::humanReadableBytesCount(bytes));
}

void ItemViewImageDelegate::drawTags(QPainter* p, const QRect& r, const QString& tagsString,
                                     bool isSelected) const
{
    Q_D(const ItemViewImageDelegate);

    p->setFont(d->fontCom);
    p->setPen(isSelected ? qApp->palette().color(QPalette::HighlightedText)
                         : qApp->palette().color(QPalette::Link));

    p->drawText(r, Qt::AlignCenter, squeezedTextCached(p, r.width(), tagsString));
}

void ItemViewImageDelegate::drawFocusRect(QPainter* p, const QStyleOptionViewItem& option,
                                          bool isSelected) const
{
    Q_D(const ItemViewImageDelegate);

    if (option.state & QStyle::State_HasFocus) //?? is current item
    {
        p->setPen(QPen(isSelected ? qApp->palette().color(QPalette::HighlightedText)
                                  : qApp->palette().color(QPalette::Text),
                       1, Qt::DotLine));
        p->drawRect(1, 1, d->rect.width()-3, d->rect.height()-3);
    }
}

void ItemViewImageDelegate::drawImageFormat(QPainter* p, const QRect& r, const QString& f, bool drawTop) const
{
    Q_D(const ItemViewImageDelegate);

    if (!f.isEmpty() && !r.isNull())
    {
        Qt::Alignment alignment = Qt::AlignBottom;

        if (drawTop)
            alignment = Qt::AlignTop;

        p->save();

        QFont fnt(d->fontReg);
        fnt.setWeight(QFont::Black);
        fnt.setItalic(false);
        p->setFont(fnt);
        p->setPen(QPen(Qt::gray));
        p->setOpacity(0.50);

        QRect bRect = p->boundingRect(r, alignment | Qt::AlignHCenter, f.toUpper());
        bRect.adjust(0, 0, 0, -2);

        if (!drawTop)
            bRect.translate(0, 1);

        p->fillRect(bRect, Qt::SolidPattern);
        p->setPen(QPen(Qt::white));
        p->setOpacity(1.0);
        p->drawText(bRect, Qt::AlignBottom | Qt::AlignHCenter, f.toUpper());

        p->restore();
    }
}

void ItemViewImageDelegate::drawPickLabelIcon(QPainter* p, const QRect& r, int pickId) const
{
    // Draw Pick Label icon
    if (pickId != NoPickLabel)
    {
        QIcon icon;

        if (pickId == RejectedLabel)
        {
            icon = QIcon::fromTheme(QLatin1String("flag-red"));
        }
        else if (pickId == PendingLabel)
        {
            icon = QIcon::fromTheme(QLatin1String("flag-yellow"));
        }
        else if (pickId == AcceptedLabel)
        {
            icon = QIcon::fromTheme(QLatin1String("flag-green"));
        }

        icon.paint(p, r);
    }
}

void ItemViewImageDelegate::drawPanelSideIcon(QPainter* p, bool left, bool right) const
{
    Q_D(const ItemViewImageDelegate);

    const int iconSize = qBound(16, d->rect.width() / 8 - 2, 48);

    if (left)
    {
        QRect r(3, d->rect.height()/2 - iconSize/2, iconSize, iconSize);
        QIcon icon = QIcon::fromTheme(QLatin1String("go-previous"));
        icon.paint(p, r);
    }

    if (right)
    {
        QRect r(d->rect.width() - 3 - iconSize, d->rect.height()/2 - iconSize/2, iconSize, iconSize);
        QIcon icon = QIcon::fromTheme(QLatin1String("go-next"));
        icon.paint(p, r);
    }
}

void ItemViewImageDelegate::drawGeolocationIndicator(QPainter* p, const QRect& r) const
{
    if (!r.isNull())
    {
        QIcon icon(QIcon::fromTheme(QLatin1String("globe")).pixmap(r.size()));
        qreal op = p->opacity();
        p->setOpacity(0.5);
        icon.paint(p, r);
        p->setOpacity(op);
    }
}

void ItemViewImageDelegate::drawGroupIndicator(QPainter* p, const QRect& r,
                                               int numberOfGroupedImages, bool open) const
{
    if (numberOfGroupedImages)
    {
        QIcon icon;

        if (open)
        {
            icon = QIcon::fromTheme(QLatin1String("folder-open")); //image-stack-open
        }
        else
        {
            icon = QIcon::fromTheme(QLatin1String("folder")); //image-stack
        }

        qreal op = p->opacity();
        p->setOpacity(0.5);
        icon.paint(p, r);
        p->setOpacity(op);

        QString text = QString::number(numberOfGroupedImages);
        p->drawText(r, Qt::AlignCenter, text);
    }
}

void ItemViewImageDelegate::drawColorLabelRect(QPainter* p, const QStyleOptionViewItem& option,
                                               bool isSelected, int colorId) const
{
    Q_D(const ItemViewImageDelegate);
    Q_UNUSED(option);
    Q_UNUSED(isSelected);

    if (colorId > NoColorLabel)
    {
        // This draw a simple rectangle around item.
        p->setPen(QPen(ColorLabelWidget::labelColor((ColorLabel)colorId), 5, Qt::SolidLine));
        p->drawRect(3, 3, d->rect.width()-7, d->rect.height()-7);
    }
}

void ItemViewImageDelegate::drawMouseOverRect(QPainter* p, const QStyleOptionViewItem& option) const
{
    Q_D(const ItemViewImageDelegate);

    if (option.state & QStyle::State_MouseOver)
    {
        p->setPen(QPen(option.palette.color(QPalette::Highlight), 3, Qt::SolidLine));
        p->drawRect(1, 1, d->rect.width()-3, d->rect.height()-3);
    }
}

void ItemViewImageDelegate::prepareFonts()
{
    Q_D(ItemViewImageDelegate);

    d->fontReg  = d->font;
    d->fontCom  = d->font;
    d->fontXtra = d->font;
    d->fontCom.setItalic(true);

    int fnSz = d->fontReg.pointSize();

    if (fnSz > 0)
    {
        d->fontCom.setPointSize(fnSz-1);
        d->fontXtra.setPointSize(fnSz-2);
    }
    else
    {
        fnSz = d->fontReg.pixelSize();
        d->fontCom.setPixelSize(fnSz-1);
        d->fontXtra.setPixelSize(fnSz-2);
    }
}

void ItemViewImageDelegate::prepareMetrics(int maxWidth)
{
    Q_D(ItemViewImageDelegate);

    QFontMetrics fm(d->fontReg);
    d->oneRowRegRect = fm.boundingRect(0, 0, maxWidth, 0xFFFFFFFF,
                                       Qt::AlignTop | Qt::AlignHCenter,
                                       QLatin1String("XXXXXXXXX"));
    fm = QFontMetrics(d->fontCom);
    d->oneRowComRect = fm.boundingRect(0, 0, maxWidth, 0xFFFFFFFF,
                                       Qt::AlignTop | Qt::AlignHCenter,
                                       QLatin1String("XXXXXXXXX"));
    fm = QFontMetrics(d->fontXtra);
    d->oneRowXtraRect = fm.boundingRect(0, 0, maxWidth, 0xFFFFFFFF,
                                        Qt::AlignTop | Qt::AlignHCenter,
                                        QLatin1String("XXXXXXXXX"));
}

void ItemViewImageDelegate::prepareBackground()
{
    Q_D(ItemViewImageDelegate);

    if (!d->rect.isValid())
    {
        d->regPixmap = QPixmap();
        d->selPixmap = QPixmap();
    }
    else
    {
        d->regPixmap = QPixmap(d->rect.width(), d->rect.height());
        d->regPixmap.fill(qApp->palette().color(QPalette::Base));
        QPainter p1(&d->regPixmap);
        p1.setPen(qApp->palette().color(QPalette::Midlight));
        p1.drawRect(0, 0, d->rect.width()-1, d->rect.height()-1);

        d->selPixmap = QPixmap(d->rect.width(), d->rect.height());
        d->selPixmap.fill(qApp->palette().color(QPalette::Highlight));
        QPainter p2(&d->selPixmap);
        p2.setPen(qApp->palette().color(QPalette::Midlight));
        p2.drawRect(0, 0, d->rect.width()-1, d->rect.height()-1);
    }
}

void ItemViewImageDelegate::prepareRatingPixmaps(bool composeOverBackground)
{
    // Please call this method after prepareBackground() and when d->ratingPixmap is set

    Q_D(ItemViewImageDelegate);

    if (!d->ratingRect.isValid())
    {
        return;
    }

    // We use antialiasing and want to pre-render the pixmaps.
    // So we need the background at the time of painting,
    // and the background may be a gradient, and will be different for selected items.
    // This makes 5*2 (small) pixmaps.

    for (int sel=0; sel<2; ++sel)
    {
        QPixmap basePix;

        if (composeOverBackground)
        {
            // do this once for regular, once for selected backgrounds
            if (sel)
            {
                basePix = d->selPixmap.copy(d->ratingRect);
            }
            else
            {
                basePix = d->regPixmap.copy(d->ratingRect);
            }
        }
        else
        {
            basePix = QPixmap(d->ratingRect.size());
            basePix.fill(Qt::transparent);
        }

        for (int rating = 1 ; rating <= 5 ; ++rating)
        {
            // we store first the 5 regular, then the 5 selected pixmaps, for simplicity
            int index = (sel * 5 + rating) - 1;

            // copy background
            d->ratingPixmaps[index] = basePix;
            // open a painter
            QPainter painter(&d->ratingPixmaps[index]);

            // use antialiasing
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setBrush(qApp->palette().color(QPalette::Link));
            QPen pen(qApp->palette().color(QPalette::Text));
            // set a pen which joins the lines at a filled angle
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);

            // move painter while drawing polygons
            painter.translate( lround((d->ratingRect.width() - d->margin - rating*(d->starPolygonSize.width()+1))/2.0) + 2, 0);

            for (int s = 0 ; s < rating ; ++s)
            {
                painter.drawPolygon(d->starPolygon, Qt::WindingFill);
                painter.translate(d->starPolygonSize.width() + 1, 0);
            }
        }
    }
}

QPixmap ItemViewImageDelegate::ratingPixmap(int rating, bool selected) const
{
    Q_D(const ItemViewImageDelegate);

    if (rating < 1 || rating > 5)
    {
        return QPixmap();
    }

    --rating;

    if (selected)
    {
        return d->ratingPixmaps.at(5 + rating);
    }
    else
    {
        return d->ratingPixmaps.at(rating);
    }
}

} // namespace Digikam
