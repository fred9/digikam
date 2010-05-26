/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-09-14
 * Description : a class to build the tooltip for a renameparser and its options
 *
 * Copyright (C) 2009 by Andi Clemens <andi dot clemens at gmx dot net>
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

#include "tooltipcreator.h"

// Qt includes

#include <QRegExp>

// KDE includes

#include <klocale.h>

// Local includes

#include "modifier.h"
#include "option.h"
#include "parser.h"
#include "themeengine.h"

using namespace Digikam;

namespace Digikam
{

TooltipCreator& TooltipCreator::getInstance()
{
    static TooltipCreator m_instance;
    return m_instance;
}

QString TooltipCreator::tooltip(Parser* parser)
{
    if (!parser)
    {
        return QString();
    }

    QString tooltip;

    tooltip += tableStart();
    tooltip += createSection(i18n("Renaming Options"), parser->options());
    tooltip += createSection(i18n("Modifiers"),        parser->modifiers(), true);
    tooltip += tableEnd();

    if (!parser->modifiers().isEmpty())
    {
        tooltip += i18n("<p><i>Modifiers can be applied to every renaming option. <br/>"
                        "It is possible to assign multiple modifiers to an option, "
                        "they are applied in the order you assign them.</i></p>");
    }

    return tooltip;
}

QString TooltipCreator::tableStart()
{
    return QString("<qt><table cellspacing=\"0\" cellpadding=\"0\" border=\"0\">");
}

QString TooltipCreator::tableEnd()
{
    return QString("</table></qt>");
}

QString TooltipCreator::markOption(const QString& str)
{
    QString result = str;

    QRegExp optionsRegExp("\\|\\|(.*)\\|\\|");
    optionsRegExp.setMinimal(true);

    result.replace(optionsRegExp, QString("<i><font color=\"%1\">\\1</font></i>")
                                          .arg(ThemeEngine::instance()->textSpecialRegColor().name()));
    return result;
}

QString TooltipCreator::createHeader(const QString& str)
{
    QString result;
    QString templateStr = QString("<tr bgcolor=\"%1\"><td colspan=\"2\">"
                                  "<nobr><font color=\"%2\"><center><b>%3"
                                  "</b></center></font></nobr></td></tr>")
                                  .arg(ThemeEngine::instance()->baseColor().name())
                                  .arg(ThemeEngine::instance()->textRegColor().name());

    result += templateStr.arg(str);
    return result;
}

template <class T>
QString TooltipCreator::createEntries(const QList<T*> &data)
{
    QString result;

    foreach (T* t, data)
    {
        foreach (Token* token, t->tokens())
        {
            result += QString("<tr>"
                              "<td bgcolor=\"%1\">"
                              "<font color=\"%2\"><b>&nbsp;%3&nbsp;</b></font></td>"
                              "<td>&nbsp;%4&nbsp;</td></tr>")
                              .arg(ThemeEngine::instance()->baseColor().name())
                              .arg(ThemeEngine::instance()->textRegColor().name())
                              .arg(markOption(token->id()))
                              .arg(markOption(token->description()));
        }
    }

    return result;
}

template <class T>
QString TooltipCreator::createSection(const QString& sectionName, const QList<T*> &data, bool lastSection)
{
    if (data.isEmpty())
    {
        return QString();
    }

    QString result;

    result += createHeader(sectionName);
    result += createEntries(data);

    if (!lastSection)
    {
        result += QString("<tr></tr>");
    }

    return result;
}

} // namespace Digikam
