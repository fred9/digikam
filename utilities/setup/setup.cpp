/* ============================================================
 * File   : setup.cpp
 * Authors: Renchi Raju <renchi at pooh.tam.uiuc.edu>
 *          Gilles Caulier <caulier dot gilles at free.fr>
 * Date   : 2003-02-03
 * Description : Digikam setup dialog.
 * 
 * Copyright 2003-2004 by Renchi Raju and Gilles Caulier
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
 
// Qt includes.

#include <qtabwidget.h>
#include <qapplication.h>
#include <qframe.h>

// KDE includes.

#include <klocale.h>
#include <kiconloader.h>

// Local includes.

#include "setupgeneral.h"
#include "setupexif.h"
#include "setupcollections.h"
#include "setupmime.h"
#include "setupeditor.h"
#include "setupplugins.h"
#include "setupcamera.h"
#include "setupmisc.h"
#include "setup.h"


Setup::Setup(QWidget* parent, const char* name, Setup::Page page)
     : KDialogBase(IconList, i18n("Configure"), Help|Ok|Cancel, Ok, parent,
                   name, true, true )
{
    setWFlags(Qt::WDestructiveClose);
    setHelp("setupdialog.anchor", "digikam");
    
    page_general = addPage(i18n("Albums"), i18n("Album Settings"),
                           BarIcon("folder_image", KIcon::SizeMedium));
    generalPage_ = new SetupGeneral(page_general, this);

    page_exif = addPage(i18n("Embedded Info"), i18n("Embedded Images Informations Settings"),
                        BarIcon("exifinfo", KIcon::SizeMedium));
    exifPage_ = new SetupExif(page_exif);
        
    page_collections = addPage(i18n("Collections"), i18n("Album Collection Settings"),
                               BarIcon("fileopen", KIcon::SizeMedium));
    collectionsPage_ = new SetupCollections(page_collections);

    page_mime = addPage(i18n("Mime Types"), i18n("Album Items' MIME Types Settings"),
                        BarIcon("mime", KIcon::SizeMedium));
    mimePage_ = new SetupMime(page_mime);

    page_editor = addPage(i18n("Image Editor"), i18n("Image Editor Settings"),
                        BarIcon("image", KIcon::SizeMedium));
    editorPage_ = new SetupEditor(page_editor);
    
    page_plugins = addPage(i18n("Kipi Plugins"), i18n("Kipi Plugin Management Settings"),
                           BarIcon("kipi", KIcon::SizeMedium));
    pluginsPage_ = new SetupPlugins(page_plugins);
        
    page_camera = addPage(i18n("Cameras"), i18n("Camera Settings"),
                          BarIcon("digitalcam", KIcon::SizeMedium));
    cameraPage_ = new SetupCamera(page_camera);

    page_misc   = addPage(i18n("Miscellaneous"), i18n("Miscellaneous Settings"),
                          BarIcon("misc", KIcon::SizeMedium));
    miscPage_ = new SetupMisc(page_misc);
    
    connect(this, SIGNAL(okClicked()),
            this, SLOT(slotOkClicked()) );

    showPage((int) page);        
    
    show();
}

Setup::~Setup()
{
}

void Setup::slotOkClicked()
{
    generalPage_->applySettings();
    collectionsPage_->applySettings();
    mimePage_->applySettings();
    cameraPage_->applySettings();
    exifPage_->applySettings();
    editorPage_->applySettings();
    miscPage_->applySettings();
    close();
}


#include "setup.moc"
