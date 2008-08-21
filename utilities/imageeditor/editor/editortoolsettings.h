/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2008-08-21
 * Description : Editor tool settings template box
 *
 * Copyright (C) 2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef RAWSETTINGSBOX_H
#define RAWSETTINGSBOX_H

// Qt includes.

#include <qwidget.h>

// Local includes.

#include "digikam_export.h"

namespace Digikam
{

class EditorToolSettingsPriv;

class DIGIKAM_EXPORT EditorToolSettings : public QWidget
{
    Q_OBJECT

public:

    EditorToolSettings(QWidget *parent);
    ~EditorToolSettings();

    virtual void setDefaultSettings()=0;
    virtual void setBusy(bool b)=0;

    virtual void saveSettings()=0;
    virtual void readSettings()=0;

signals:

    void signalOkClicked();
    void signalCancelClicked();
    void signalTryClicked();
    void signalDefaultClicked();

private:

    EditorToolSettingsPriv *d;
};

} // NameSpace Digikam

#endif // RAWSETTINGSBOX_H
