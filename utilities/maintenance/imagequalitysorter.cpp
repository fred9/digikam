/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2013-08-19
 * Description : image quality sorter
 *
 * Copyright (C) 2013 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "imagequalitysorter.moc"

// Qt includes

#include <QString>

// KDE includes

#include <klocale.h>

// Local includes

#include "dimg.h"
#include "albumdb.h"
#include "albummanager.h"
#include "databaseaccess.h"
#include "metadatasettings.h"
#include "maintenancethread.h"

namespace Digikam
{

class ImageQualitySorter::Private
{
public:

    Private() :
        rebuildAll(true),
        quality(0),             // FIXME : use right default value here.
        thread(0)
    {
    }

    bool               rebuildAll;
    
    int                quality;

    QStringList        allPicturesPath;

    AlbumList          albumList;

    MaintenanceThread* thread;
};

ImageQualitySorter::ImageQualitySorter(const bool rebuildAll, const AlbumList& list,
                                       int const quality, ProgressItem* const parent)
    : MaintenanceTool("ImageQualitySorter", parent),
      d(new Private)
{
    setLabel(i18n("Image Quality Sorter"));
    ProgressManager::addProgressItem(this);

    d->rebuildAll = rebuildAll;
    d->albumList  = list;
    d->quality    = quality;
    d->thread     = new MaintenanceThread(this);

    connect(d->thread, SIGNAL(signalCompleted()),
            this, SLOT(slotDone()));

    connect(d->thread, SIGNAL(signalAdvance(QImage)),
            this, SLOT(slotAdvance(QImage)));
}

ImageQualitySorter::~ImageQualitySorter()
{
    delete d;
}

void ImageQualitySorter::setUseMultiCoreCPU(bool b)
{
    d->thread->setUseMultiCore(b);
}

void ImageQualitySorter::slotCancel()
{
    d->thread->cancel();
    MaintenanceTool::slotCancel();
}

void ImageQualitySorter::slotStart()
{
    MaintenanceTool::slotStart();

    if (d->albumList.isEmpty())
    {
        d->albumList = AlbumManager::instance()->allPAlbums();
    }

    // Get all digiKam albums collection pictures path, depending of d->rebuildAll flag.

    for (AlbumList::ConstIterator it = d->albumList.constBegin();
         !canceled() && (it != d->albumList.constEnd()); ++it)
    {
        d->allPicturesPath += DatabaseAccess().db()->getItemURLsInAlbum((*it)->id());

        if (!d->rebuildAll)
        {
            // TODO : Add a new method to get items list with no pick label assigned in DB.
            /*QStringList dirty = DatabaseAccess().db()->getDirtyOrMissingFingerprintURLs();

            foreach(QString path, dirty)
            {
                if (dirty.contains(path))
                {
                    d->allPicturesPath.removeAll(path);
                }
            }*/
        }
    }

    if (d->allPicturesPath.isEmpty())
    {
        slotDone();
        return;
    }

    setTotalItems(d->allPicturesPath.count());

    d->thread->sortByImageQuality(d->allPicturesPath, d->quality);
    d->thread->start();
}

void ImageQualitySorter::slotAdvance(const QImage& img)
{
    setThumbnail(QPixmap::fromImage(img));
    advance(1);
}

}  // namespace Digikam
