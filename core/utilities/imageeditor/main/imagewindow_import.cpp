/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-11-22
 * Description : digiKam image editor - Import tools
 *
 * Copyright (C) 2004-2018 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "imagewindow.h"
#include "imagewindow_p.h"

namespace Digikam
{

void ImageWindow::slotImportFromScanner()
{
#ifdef HAVE_KSANE
    m_ksaneAction->activate(DigikamApp::instance()->scannerTargetPlace(), configGroupName());

    connect(m_ksaneAction, SIGNAL(signalImportedImage(QUrl)),
            this, SLOT(slotImportedImagefromScanner(QUrl)));
#endif
}

void ImageWindow::slotImportedImagefromScanner(const QUrl& url)
{
    ImageInfo info = ScanController::instance()->scannedInfo(url.toLocalFile());
    openImage(info);
}

void ImageWindow::slotImportTool()
{
    QAction* const tool = dynamic_cast<QAction*>(sender());

    if (tool == m_importGphotoAction)
    {
        QPointer<GSWindow> w = new GSWindow(new DBInfoIface(this, QList<QUrl>(), ApplicationSettings::ImportExport),
                   this, QLatin1String("googlephotoimport"));
        w->exec();
        delete w;
    }
    else if (tool == m_importSmugmugAction)
    {
        QPointer<SmugWindow> w = new SmugWindow(new DBInfoIface(this, QList<QUrl>(), ApplicationSettings::ImportExport),
                     this, true);
        w->exec();
        delete w;
    }

#ifdef HAVE_KIO
    else if (tool == m_importFileTransferAction)
    {
        QPointer<FTImportWindow> w = new FTImportWindow(new DBInfoIface(this, QList<QUrl>(), ApplicationSettings::ImportExport),
                         this);
        w->exec();
        delete w;
    }
#endif
}

} // namespace Digikam