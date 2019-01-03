/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2018-07-30
 * Description : a plugin to generate HTML image galleries.
 *
 * Copyright (C) 2018-2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "htmlgalleryplugin.h"

// Qt includes

#include <QPointer>
#include <QIcon>

// KDE includes

#include <klocalizedstring.h>

// Local includes

#include "htmlwizard.h"

namespace Digikam
{

HtmlGalleryPlugin::HtmlGalleryPlugin(QObject* const parent)
    : DPlugin(parent)
{
}

QString HtmlGalleryPlugin::name() const
{
    return i18n("Html Gallery");
}

QString HtmlGalleryPlugin::id() const
{
    return QLatin1String("HtmlGallery");
}

QString HtmlGalleryPlugin::version() const
{
    return QLatin1String("1.0");
}

QString HtmlGalleryPlugin::description() const
{
    return i18n("A Tool to generate HTML image galleries");
}

QString HtmlGalleryPlugin::details() const
{
    return i18n("<p>This tool permit to back-process items (as resize) before to create W3C compliant html gallery.</p>"
                "<p>Items to process can be selected one by one or by group through a selection of albums.</p>"
                "<p>Themable HTML template with different layout can be used to assemble files on a gallery.</p>");
}

QList<DPluginAuthor> HtmlGalleryPlugin::authors() const
{
    return QList<DPluginAuthor>()
            << DPluginAuthor(QLatin1String("Gilles Caulier"),
                             QLatin1String("caulier dot gilles at gmail dot com"),
                             QLatin1String("(C) 2006-2019"),
                             i18n("Developer and Maintainer"))
            << DPluginAuthor(QLatin1String("Aurelien Gateau"),
                             QLatin1String("agateau at kde dot org"),
                             QLatin1String("(C) 2006-2009"),
                             i18n("Former Author and Maintainer"))
            << DPluginAuthor(QLatin1String("Gianluca Urgese"),
                             QLatin1String("giasone dot 82 at gmail dot com"),
                             QLatin1String("(C) 2010"))
            ;
}

void HtmlGalleryPlugin::setup()
{
    DPluginAction* const ac = new DPluginAction(this);
    ac->setIcon(QIcon::fromTheme(QLatin1String("text-html")));
    ac->setText(i18nc("@action", "Create Html gallery..."));
    ac->setActionName(QLatin1String("htmlgallery"));
    ac->setShortcut(Qt::ALT + Qt::SHIFT + Qt::Key_H);
    ac->setActionCategory(DPluginAction::GenericTool);

    connect(ac, SIGNAL(triggered(bool)),
            this, SLOT(slotHtmlGallery()));

    addAction(ac);
}

void HtmlGalleryPlugin::slotHtmlGallery()
{
    QPointer<HTMLWizard> wzrd = new HTMLWizard(0, infoIface());
    wzrd->setPlugin(this);
    wzrd->exec();
    delete wzrd;
}

} // namespace Digikam