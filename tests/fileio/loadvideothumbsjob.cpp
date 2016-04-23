/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2016-04-21
 * Description : a class to manage video thumbnails extraction
 *
 * Copyright (C) 2016 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "loadvideothumbsjob.h"

// Qt includes

#include <QImage>
#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

// Local includes

#include "loadvideothumb.h"

class LoadVideoThumbsJob::Private
{
public:

    Private()
    {
        vthumb = 0;
    }

    bool              canceled;
    bool              running;

    QMutex            mutex;
    QWaitCondition    condVar;

    QStringList       todo;
    QString           currentFile;
    VideoThumbnailer* vthumb;
};

LoadVideoThumbsJob::LoadVideoThumbsJob(QObject* const parent)
    : QThread(parent),
      d(new Private)
{
    d->vthumb = new VideoThumbnailer(this);

    connect(this, SIGNAL(signalGetThumbnail(const QString&)),
            d->vthumb, SLOT(slotGetThumbnail(const QString&)));

    connect(d->vthumb, SIGNAL(signalThumbnailDone(const QString&, const QImage&)),
            this, SLOT(slotThumbnailDone(const QString&, const QImage&)));

    connect(d->vthumb, SIGNAL(signalThumbnailFailed(const QString&)),
            this, SLOT(slotThumbnailFailed(const QString&)));
}

LoadVideoThumbsJob::~LoadVideoThumbsJob()
{
    // clear updateItems, stop processing
    slotCancel();

    // stop thread
    {
        QMutexLocker lock(&d->mutex);
        d->running = false;
        d->condVar.wakeAll();
    }

    wait();

    delete d;
}

void LoadVideoThumbsJob::setThumbnailSize(int size)
{
    d->vthumb->setThumbnailSize(size);
}

void LoadVideoThumbsJob::setCreateStrip(bool strip)
{
    d->vthumb->setCreateStrip(strip);
}

void LoadVideoThumbsJob::slotCancel()
{
    d->running = false;
    QMutexLocker lock(&d->mutex);
    d->todo.clear();
    d->currentFile.clear();
}

void LoadVideoThumbsJob::addItems(const QStringList& files)
{
    if (files.isEmpty())
    {
        return;
    }

    {
        QMutexLocker lock(&d->mutex);
        d->running = true;
        d->todo << files;

        if (!isRunning())
        {
            start(LowPriority);
        }
    }

    processOne();
}

void LoadVideoThumbsJob::processOne()
{
    QMutexLocker lock(&d->mutex);

    if (!d->todo.isEmpty())
    {
        d->currentFile = d->todo.takeFirst();
        qDebug() << "Add " << d->currentFile << " to the todo list";
        d->condVar.wakeAll();
    }
    else
    {
        emit signalComplete();
    }
}

void LoadVideoThumbsJob::run()
{
    while (d->running)
    {
        QMutexLocker lock(&d->mutex);

        if (!d->currentFile.isNull())
        {
            qDebug() << "Request to get thumbnail for " << d->currentFile;
            emit signalGetThumbnail(d->currentFile);
            d->currentFile.clear();
        }
        else
        {
            d->condVar.wait(&d->mutex);
            continue;
        }
    }

    emit signalComplete();
}

void LoadVideoThumbsJob::slotThumbnailDone(const QString& file, const QImage& img)
{
    emit signalThumbnailDone(file, img);
    qDebug() << "Video thumbnail extracted for " << file;
    processOne();
}

void LoadVideoThumbsJob::slotThumbnailFailed(const QString& file)
{
    emit signalThumbnailFailed(file);
    qDebug() << "Video thumbnail failed for " << file;
    processOne();
}