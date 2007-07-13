/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-10-04
 * Description : sync IO jobs.
 * 
 * Copyright (C) 2004 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 *
 * Concept copied from kdelibs/kio/kio/netaccess.h/cpp
 *   This file is part of the KDE libraries
 *   Copyright (C) 1997 Torben Weis (weis@kde.org)
 *   Copyright (C) 1998 Matthias Ettrich (ettrich@kde.org)
 *   Copyright (C) 1999 David Faure (faure@kde.org)
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

#ifndef SYNCJOB_H
#define SYNCJOB_H

// Qt includes.

#include <QObject>
#include <QPixmap>

// KDe includes.

#include <kurl.h>

class QString;
class QEventLoop;

namespace KIO
{
class Job;
}

namespace Digikam
{

class Album;
class TAlbum;

class SyncJob : public QObject
{
    Q_OBJECT

public:

    /* this will delete the urls. */
    static bool del(const KUrl::List& urls, bool useTrash);

    /* remove this when we move dependency upto kde 3.2 */
    static bool file_move(const KUrl &src, const KUrl &dest);

    /* Load the image or icon for the tag thumbnail */    
    static QPixmap getTagThumbnail(TAlbum *album);
    static QPixmap getTagThumbnail(const QString &name, int size);

    static QString lastErrorMsg();
    static int     lastErrorCode();
    
private:

    SyncJob();
    ~SyncJob();

    void enterWaitingLoop();
    void quitWaitingLoop();

    bool delPriv(const KUrl::List& urls);
    bool trashPriv(const KUrl::List& urls);

    bool fileMovePriv(const KUrl &src, const KUrl &dest);
    
    QPixmap getTagThumbnailPriv(TAlbum *album);
    QPixmap getTagThumbnailPriv(const QString &name, int size);

private slots:

    void slotResult( KIO::Job * job );
    void slotGotThumbnailFromIcon(Album *album, const QPixmap& pix);
    void slotLoadThumbnailFailed(Album *album);
    void slotGotThumbnailFromIcon(const KUrl& url, const QPixmap& pix);
    void slotLoadThumbnailFailed();

private:

    static int       lastErrorCode_;
    static QString  *lastErrorMsg_;
    bool             success_;
    
    int              thumbnailSize_;
    Album           *album_;
    QPixmap         *thumbnail_;

    QEventLoop      *waitingLoop_;
};

}  // namespace Digikam

#endif /* SYNCJOB_H */
