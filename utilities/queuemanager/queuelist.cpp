/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-11-21
 * Description : Batch Queue Manager items list.
 *
 * Copyright (C) 2008-2009 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#define ICONSIZE 64

#include "queuelist.h"
#include "queuelist.moc"

// Qt includes.

#include <QPainter>
#include <QDragEnterEvent>
#include <QUrl>
#include <QFileInfo>
#include <QHeaderView>

// KDE includes.

#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>

// Local includes.

#include "albumdb.h"
#include "thumbnailsize.h"
#include "thumbnailloadthread.h"
#include "ddragobjects.h"

namespace Digikam
{

class QueueListViewItemPriv
{

public:

    QueueListViewItemPriv()
    {
        done = false;
    }

    bool      done;

    QString   destFileName;

    ImageInfo info;
};

QueueListViewItem::QueueListViewItem(QTreeWidget *view, const ImageInfo& info)
                 : QTreeWidgetItem(view), d(new QueueListViewItemPriv)
{
    setThumb(SmallIcon("image-x-generic", KIconLoader::SizeLarge, KIconLoader::DisabledState));
    setInfo(info);
}

QueueListViewItem::~QueueListViewItem()
{
    delete d;
}

void QueueListViewItem::setInfo(const ImageInfo& info)
{
    d->info = info;
    setText(1, d->info.name());
}

ImageInfo QueueListViewItem::info() const
{
    return d->info;
}

void QueueListViewItem::setThumb(const QPixmap& pix)
{
    QPixmap pixmap(ICONSIZE+2, ICONSIZE+2);
    pixmap.fill(Qt::color0);
    QPainter p(&pixmap);
    p.drawPixmap((pixmap.width()/2) - (pix.width()/2), (pixmap.height()/2) - (pix.height()/2), pix);
    setIcon(0, QIcon(pixmap));
}

void QueueListViewItem::setProgressIcon(const QIcon& icon)
{
    setIcon(1, icon);
}

void QueueListViewItem::setDone(bool done)
{
    setProgressIcon(SmallIcon("dialog-ok"));
    d->done = done;
}

bool QueueListViewItem::isDone()
{
    return d->done;
}

void QueueListViewItem::setDestFileName(const QString& str)
{
    d->destFileName = str;
    setText(2, d->destFileName);
}

QString QueueListViewItem::destFileName() const
{
    return d->destFileName;
}

// ---------------------------------------------------------------------------

class QueueListViewPriv
{

public:

    enum RemoveItemsType
    {
        ItemsSelected = 0,
        ItemsDone,
        ItemsAll
    };

public:

    QueueListViewPriv()
    {
        thumbLoadThread = ThumbnailLoadThread::defaultThread();
    }

    ThumbnailLoadThread *thumbLoadThread;

    QueueSettings        settings;

    AssignedBatchTools   toolsList;
};

QueueListView::QueueListView(QWidget *parent)
             : QTreeWidget(parent), d(new QueueListViewPriv)
{
    setIconSize(QSize(ICONSIZE, ICONSIZE));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setWhatsThis(i18n("This is the list of images to batch process."));

    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragEnabled(true);

    setSortingEnabled(false);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setColumnCount(3);

    QStringList titles;
    titles.append(i18n("Thumbnail"));
    titles.append(i18n("File Name"));
    titles.append(i18n("Target"));
    setHeaderLabels(titles);
    header()->setResizeMode(0, QHeaderView::ResizeToContents);
    header()->setResizeMode(1, QHeaderView::Stretch);
    header()->setResizeMode(2, QHeaderView::Stretch);

    connect(d->thumbLoadThread, SIGNAL(signalThumbnailLoaded(const LoadingDescription&, const QPixmap&)),
            this, SLOT(slotThumbnailLoaded(const LoadingDescription&, const QPixmap&)));
}

QueueListView::~QueueListView()
{
    delete d;
}

Qt::DropActions QueueListView::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData* QueueListView::mimeData(const QList<QTreeWidgetItem*> items) const
{
    KUrl::List urls;
    KUrl::List kioURLs;
    QList<int> albumIDs;
    QList<int> imageIDs;

    foreach(QTreeWidgetItem* itm, items)
    {
        QueueListViewItem* vitem = dynamic_cast<QueueListViewItem*>(itm);
        if (vitem)
        {
            urls.append(vitem->info().fileUrl());
            kioURLs.append(vitem->info().databaseUrl());
            albumIDs.append(vitem->info().albumId());
            imageIDs.append(vitem->info().id());
        }
    }

    DItemDrag *mimeData = new DItemDrag(urls, kioURLs, albumIDs, imageIDs);
    return mimeData;
}

void QueueListView::startDrag(Qt::DropActions /*supportedActions*/)
{
    QList<QTreeWidgetItem*> items = selectedItems();
    if (items.isEmpty())
        return;

    QPixmap icon(DesktopIcon("image-jp2", 48));
    int w = icon.width();
    int h = icon.height();

    QPixmap pix(w+4,h+4);
    QString text(QString::number(items.count()));

    QPainter p(&pix);
    p.fillRect(0, 0, pix.width()-1, pix.height()-1, QColor(Qt::white));
    p.setPen(QPen(Qt::black, 1));
    p.drawRect(0, 0, pix.width()-1, pix.height()-1);
    p.drawPixmap(2, 2, icon);
    QRect r = p.boundingRect(2, 2, w, h, Qt::AlignLeft|Qt::AlignTop, text);
    r.setWidth(qMax(r.width(), r.height()));
    r.setHeight(qMax(r.width(), r.height()));
    p.fillRect(r, QColor(0, 80, 0));
    p.setPen(Qt::white);
    QFont f(font());
    f.setBold(true);
    p.setFont(f);
    p.drawText(r, Qt::AlignCenter, text);
    p.end();

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData(items));
    drag->setPixmap(pix);
    drag->exec();
}

void QueueListView::dragEnterEvent(QDragEnterEvent *e)
{
    QTreeWidget::dragEnterEvent(e);
    e->accept();
}

void QueueListView::dragMoveEvent(QDragMoveEvent *e)
{
    int        albumID;
    QList<int> albumIDs;
    QList<int> imageIDs;
    KUrl::List urls;
    KUrl::List kioURLs;

    if (DItemDrag::decode(e->mimeData(), urls, kioURLs, albumIDs, imageIDs) ||
        DAlbumDrag::decode(e->mimeData(), urls, albumID) ||
        DTagDrag::canDecode(e->mimeData()))
    {
        if (DItemDrag::decode(e->mimeData(), urls, kioURLs, albumIDs, imageIDs))
        {
            ImageInfoList imageInfoList;

            for (QList<int>::const_iterator it = imageIDs.constBegin();
                it != imageIDs.constEnd(); ++it)
            {
                ImageInfo info(*it);
                if (!findItemByInfo(info))
                    imageInfoList.append(info);
            }

            if (!imageInfoList.isEmpty())
            {
                QTreeWidget::dragMoveEvent(e);
                e->accept();
                return;
            }
        }
    }
    e->ignore();
}

void QueueListView::dropEvent(QDropEvent *e)
{
    int        albumID;
    QList<int> albumIDs;
    QList<int> imageIDs;
    KUrl::List urls;
    KUrl::List kioURLs;

    if (DItemDrag::decode(e->mimeData(), urls, kioURLs, albumIDs, imageIDs))
    {
        ImageInfoList imageInfoList;

        for (QList<int>::const_iterator it = imageIDs.constBegin();
             it != imageIDs.constEnd(); ++it)
        {
            ImageInfo info(*it);
            if (!findItemByInfo(info))
            {
                imageInfoList.append(info);
            }
        }

        if (!imageInfoList.isEmpty())
        {
            slotAddItems(imageInfoList, imageInfoList.first());
            e->acceptProposedAction();

            QueueListView* vitem = dynamic_cast<QueueListView*>(e->source());
            if (vitem && vitem != this)
            {
                foreach(ImageInfo info, imageInfoList)
                    vitem->removeItemByInfo(info);
            }
        }
    }
    else if (DAlbumDrag::decode(e->mimeData(), urls, albumID))
    {
        QList<qlonglong> itemIDs = DatabaseAccess().db()->getItemIDsInAlbum(albumID);
        ImageInfoList imageInfoList;

        for (QList<qlonglong>::const_iterator it = itemIDs.constBegin();
             it != itemIDs.constEnd(); ++it)
        {
            ImageInfo info(*it);
            if (!findItemByInfo(info))
            {
                imageInfoList.append(info);
            }
        }

        if (!imageInfoList.isEmpty())
        {
            slotAddItems(imageInfoList, imageInfoList.first());
            e->acceptProposedAction();
        }
    }
    else if(DTagDrag::canDecode(e->mimeData()))
    {
        int tagID;
        if (!DTagDrag::decode(e->mimeData(), tagID))
            return;

        QList<qlonglong> itemIDs = DatabaseAccess().db()->getItemIDsInTag(tagID, true);
        ImageInfoList imageInfoList;

        for (QList<qlonglong>::const_iterator it = itemIDs.constBegin();
             it != itemIDs.constEnd(); ++it)
        {
            ImageInfo info(*it);
            if (!findItemByInfo(info))
            {
                imageInfoList.append(info);
            }
        }

        if (!imageInfoList.isEmpty())
        {
            slotAddItems(imageInfoList, imageInfoList.first());
            e->acceptProposedAction();
        }
    }
    else
    {
        e->ignore();
    }

    emit signalQueueContentsChanged();
}

void QueueListView::slotAddItems(const ImageInfoList& list, const ImageInfo& current)
{
    if ( list.count() == 0 ) return;

    for( ImageInfoList::ConstIterator it = list.begin(); it != list.end(); ++it )
    {
        ImageInfo info = *it;

        // Check if the new item already exist in the list.

        bool find               = false;
        QueueListViewItem *item = 0;

        QTreeWidgetItemIterator iter(this);
        while (*iter)
        {
            item = dynamic_cast<QueueListViewItem*>(*iter);

            if (item->info() == info)
                find = true;

            ++iter;
        }

        if (!find)
        {
            item = new QueueListViewItem(this, info);
            d->thumbLoadThread->find(info.fileUrl().path());
        }

        if (info == current)
        {
            setCurrentItem(item);
        }
    }
    emit signalQueueContentsChanged();
}

void QueueListView::slotThumbnailLoaded(const LoadingDescription& desc, const QPixmap& pix)
{
    QTreeWidgetItemIterator it(this);
    while (*it)
    {
        QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
        if (item->info().fileUrl() == KUrl(desc.filePath))
        {
            if (pix.isNull())
                item->setThumb(SmallIcon("image-x-generic", ICONSIZE, KIconLoader::DisabledState));
            else
                item->setThumb(pix.scaled(ICONSIZE, ICONSIZE, Qt::KeepAspectRatio));

            return;
        }
        ++it;
    }
}

void QueueListView::slotClearList()
{
    removeItems(QueueListViewPriv::ItemsAll);
    emit signalQueueContentsChanged();
}

void QueueListView::slotRemoveSelectedItems()
{
    removeItems(QueueListViewPriv::ItemsSelected);
    emit signalQueueContentsChanged();
}

void QueueListView::slotRemoveItemsDone()
{
    removeItems(QueueListViewPriv::ItemsDone);
    emit signalQueueContentsChanged();
}

void QueueListView::removeItems(int removeType)
{
    bool find;
    do
    {
        find = false;
        QTreeWidgetItemIterator it(this);
        while (*it)
        {
            QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
            switch(removeType)
            {
                case QueueListViewPriv::ItemsSelected:
                {
                    if (item->isSelected())
                    {
                        delete item;
                        find = true;
                    }
                    break;
                }
                case QueueListViewPriv::ItemsDone:
                {
                    if (item->isDone())
                    {
                        delete item;
                        find = true;
                    }
                    break;
                }
                default:  // ItemsAll
                {
                    delete item;
                    find = true;
                    break;
                }
            }
            ++it;
        }
    }
    while(find);

    emit signalQueueContentsChanged();
}

void QueueListView::removeItemByInfo(const ImageInfo& info)
{
    bool find;
    do
    {
        find = false;
        QTreeWidgetItemIterator it(this);
        while (*it)
        {
            QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
            if (item->info() == info)
            {
                delete item;
                find = true;
                break;
            }
            ++it;
        }
    }
    while(find);

    emit signalQueueContentsChanged();
}

bool QueueListView::findItemByInfo(const ImageInfo& info)
{
    QTreeWidgetItemIterator it(this);
    while (*it)
    {
        QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
        if (item->info() == info)
            return true;

        ++it;
    }

    return false;
}

QueueListViewItem* QueueListView::findItemByUrl(const KUrl& url)
{
    QTreeWidgetItemIterator it(this);
    while (*it)
    {
        QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
        if (item->info().fileUrl() == url)
            return item;

        ++it;
    }

    return 0;
}

int QueueListView::itemsCount()
{
    int count = 0;
    QTreeWidgetItemIterator it(this);
    while (*it)
    {
        QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
        if (item) count++;
        ++it;
    }
    return count;
}

ImageInfoList QueueListView::pendingItemsList()
{
    ImageInfoList list;
    QTreeWidgetItemIterator it(this);
    while (*it)
    {
        QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
        if (item && !item->isDone())
            list.append(item->info());
        ++it;
    }
    return list;
}

int QueueListView::pendingItemsCount()
{
    return pendingItemsList().count();
}

int QueueListView::pendingTasksCount()
{
    int count = 0;
    QTreeWidgetItemIterator it(this);
    while (*it)
    {
        QueueListViewItem* item = dynamic_cast<QueueListViewItem*>(*it);
        if (item && !item->isDone())
            count += assignedTools().toolsMap.count();
        ++it;
    }
    return count;
}

void QueueListView::setSettings(const QueueSettings& settings)
{
    d->settings = settings;
}

QueueSettings QueueListView::settings()
{
    return d->settings;
}

AssignedBatchTools QueueListView::assignedTools()
{
    return d->toolsList;
}

void QueueListView::setAssignedTools(const AssignedBatchTools& tools)
{
    d->toolsList = tools;
}

void QueueListView::slotAssignedToolsChanged(const AssignedBatchTools& tools)
{
    setAssignedTools(tools);
}

}  // namespace Digikam
