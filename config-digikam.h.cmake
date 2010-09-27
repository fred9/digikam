/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-08-03
 * Description : digiKam config header
 *
 * Copyright (C) 2009-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef CONFIG_DIGIKAM_H
#define CONFIG_DIGIKAM_H

/* Define to 1 if you have KDEPIM shared library installed */
#cmakedefine HAVE_KDEPIMLIBS 1

/* Define to 1 if you have Marble Widget shared library installed */
#cmakedefine HAVE_MARBLEWIDGET 1

/* Define to 1 if Glib2 shared library is installed */
#cmakedefine HAVE_GLIB2 1

/* Define to 1 if an external liblqr-1 shared library have been found */
#cmakedefine USE_EXT_LIBLQR-1 1

/* Define to 1 if an external lensfun shared library have been found */
#cmakedefine USE_EXT_LIBLENSFUN 1

/* Define to 1 if GPhoto2 shared library is installed */
#cmakedefine HAVE_GPHOTO2 1

/* Define to 1 if you want to use the experimental thumbnails database */
#cmakedefine USE_THUMBS_DB 1

/* Define to 1 if you have Nepomuk shared libraries installed */
#cmakedefine HAVE_NEPOMUK 1

#define LIBEXEC_INSTALL_DIR "${LIBEXEC_INSTALL_DIR}"

/* debug area codes */
#define AREACODE_GENERAL        ${AREA_CODE_GENERAL}
#define AREACODE_KIOSLAVES      ${AREA_CODE_KIOSLAVES}
#define AREACODE_SHOWFOTO       ${AREA_CODE_SHOWFOTO}
#define AREACODE_IMAGEPLUGINS   ${AREA_CODE_IMAGEPLUGINS}
#define AREACODE_DATABASESERVER ${AREA_CODE_DATABASESERVER}

#endif /* CONFIG_DIGIKAM_H */
