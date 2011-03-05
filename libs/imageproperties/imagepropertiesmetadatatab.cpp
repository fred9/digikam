/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-11-17
 * Description : a tab to display metadata information of images
 *
 * Copyright (C) 2004-2009 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "imagepropertiesmetadatatab.moc"

// Qt includes

#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QFileInfo>
#include <QVBoxLayout>

// KDE includes


#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kfileitem.h>
#include <kglobal.h>

// Local includes

#include "metadatapanel.h"
#include "exifwidget.h"
#include "makernotewidget.h"
#include "iptcwidget.h"
#include "xmpwidget.h"

namespace Digikam
{

class ImagePropertiesMetadataTabPriv
{
public:

    enum MetadataTab
    {
        EXIF=0,
        MAKERNOTE,
        IPTC,
        XMP
    };

    ImagePropertiesMetadataTabPriv() :
        exifWidget(0),
        makernoteWidget(0),
        iptcWidget(0),
        xmpWidget(0)
    {
    }

    ExifWidget*      exifWidget;
    MakerNoteWidget* makernoteWidget;
    IptcWidget*      iptcWidget;
    XmpWidget*       xmpWidget;
};

ImagePropertiesMetaDataTab::ImagePropertiesMetaDataTab(QWidget* parent)
    : KTabWidget(parent), d(new ImagePropertiesMetadataTabPriv)
{
    // Exif tab area ---------------------------------------

    d->exifWidget = new ExifWidget(this);
    insertTab(ImagePropertiesMetadataTabPriv::EXIF, d->exifWidget, i18n("EXIF"));

    // Makernote tab area ----------------------------------

    d->makernoteWidget = new MakerNoteWidget(this);
    insertTab(ImagePropertiesMetadataTabPriv::MAKERNOTE, d->makernoteWidget, i18n("Makernote"));

    // IPTC tab area ---------------------------------------

    d->iptcWidget = new IptcWidget(this);
    insertTab(ImagePropertiesMetadataTabPriv::IPTC, d->iptcWidget, i18n("IPTC"));

    // XMP tab area ----------------------------------------

    d->xmpWidget = new XmpWidget(this);

    if (DMetadata::supportXmp())
    {
        insertTab(ImagePropertiesMetadataTabPriv::XMP, d->xmpWidget, i18n("XMP"));
    }
    else
    {
        d->xmpWidget->hide();
    }
}

ImagePropertiesMetaDataTab::~ImagePropertiesMetaDataTab()
{
    delete d;
}

void ImagePropertiesMetaDataTab::readSettings(const KConfigGroup& group)
{
    setCurrentIndex(group.readEntry("ImagePropertiesMetaData Tab",
                                    (int)ImagePropertiesMetadataTabPriv::EXIF));
    d->exifWidget->setMode(group.readEntry("EXIF Level",                              (int)ExifWidget::CUSTOM));
    d->makernoteWidget->setMode(group.readEntry("MAKERNOTE Level",                    (int)MakerNoteWidget::CUSTOM));
    d->iptcWidget->setMode(group.readEntry("IPTC Level",                              (int)IptcWidget::CUSTOM));
    d->xmpWidget->setMode(group.readEntry("XMP Level",                                (int)XmpWidget::CUSTOM));
    d->exifWidget->setCurrentItemByKey(group.readEntry("Current EXIF Item",           QString()));
    d->makernoteWidget->setCurrentItemByKey(group.readEntry("Current MAKERNOTE Item", QString()));
    d->iptcWidget->setCurrentItemByKey(group.readEntry("Current IPTC Item",           QString()));
    d->xmpWidget->setCurrentItemByKey(group.readEntry("Current XMP Item",             QString()));
    d->exifWidget->setTagsFilter(group.readEntry("EXIF Tags Filter",                  MetadataPanel::defaultExifFilter()));
    d->makernoteWidget->setTagsFilter(group.readEntry("MAKERNOTE Tags Filter",        MetadataPanel::defaultMknoteFilter()));
    d->iptcWidget->setTagsFilter(group.readEntry("IPTC Tags Filter",                  MetadataPanel::defaultIptcFilter()));
    d->xmpWidget->setTagsFilter(group.readEntry("XMP Tags Filter",                    MetadataPanel::defaultXmpFilter()));
}

void ImagePropertiesMetaDataTab::writeSettings(KConfigGroup& group)
{
    group.writeEntry("ImagePropertiesMetaData Tab", currentIndex());
    group.writeEntry("EXIF Level",                  d->exifWidget->getMode());
    group.writeEntry("MAKERNOTE Level",             d->makernoteWidget->getMode());
    group.writeEntry("IPTC Level",                  d->iptcWidget->getMode());
    group.writeEntry("XMP Level",                   d->xmpWidget->getMode());
    group.writeEntry("Current EXIF Item",           d->exifWidget->getCurrentItemKey());
    group.writeEntry("Current MAKERNOTE Item",      d->makernoteWidget->getCurrentItemKey());
    group.writeEntry("Current IPTC Item",           d->iptcWidget->getCurrentItemKey());
    group.writeEntry("Current XMP Item",            d->xmpWidget->getCurrentItemKey());
}

void ImagePropertiesMetaDataTab::setCurrentURL(const KUrl& url)
{
    if (url.isEmpty())
    {
        d->exifWidget->loadFromURL(url);
        d->makernoteWidget->loadFromURL(url);
        d->iptcWidget->loadFromURL(url);
        d->xmpWidget->loadFromURL(url);
        setEnabled(false);
        return;
    }

    setEnabled(true);
    DMetadata metadata(url.toLocalFile());

    d->exifWidget->loadFromData(url.fileName(), metadata);
    d->makernoteWidget->loadFromData(url.fileName(), metadata);
    d->iptcWidget->loadFromData(url.fileName(), metadata);
    d->xmpWidget->loadFromData(url.fileName(), metadata);
}

void ImagePropertiesMetaDataTab::setCurrentData(const DMetadata& metaData, const QString& filename)
{
    DMetadata data = metaData;

    if (!data.hasExif() && !data.hasIptc() && !data.hasXmp())
    {
        d->exifWidget->loadFromData(filename, data);
        d->makernoteWidget->loadFromData(filename, data);
        d->iptcWidget->loadFromData(filename, data);
        d->xmpWidget->loadFromData(filename, data);
        setEnabled(false);
        return;
    }

    setEnabled(true);

    d->exifWidget->loadFromData(filename, data);
    d->makernoteWidget->loadFromData(filename, data);
    d->iptcWidget->loadFromData(filename, data);
    d->xmpWidget->loadFromData(filename, data);
}

}  // namespace Digikam
