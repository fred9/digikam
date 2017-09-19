/*
 *  Copyright (C) 2011 Tuomo Penttinen, all rights reserved.
 *
 *  Author: Tuomo Penttinen <tp@herqq.org>
 *
 *  This file is part of an application named HUpnpAvSimpleTestApp
 *  used for demonstrating how to use the Herqq UPnP A/V (HUPnPAv) library.
 *
 *  HUpnpAvSimpleTestApp is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HUpnpAvSimpleTestApp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HUpnpAvSimpleTestApp. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mediarenderer_connectionwindow.h"
#include "ui_mediarenderer_connectionwindow.h"
#include "digikam_config.h"

#include <QtCore/QUrl>
#include <QtGui/QCloseEvent>
#include <QtNetwork/QNetworkAccessManager>

using namespace Herqq::Upnp::Av;

namespace
{
bool isAudio(const QString& contentFormat)
{
    return contentFormat.startsWith(QLatin1String("audio"), Qt::CaseInsensitive);
}

bool isVideo(const QString& contentFormat)
{
    return contentFormat.startsWith(QLatin1String("video"), Qt::CaseInsensitive);
}

bool isImage(const QString& contentFormat)
{
    return contentFormat.startsWith(QLatin1String("image"), Qt::CaseInsensitive);
}

bool isText(const QString& contentFormat)
{
    return contentFormat.startsWith(QLatin1String("text"), Qt::CaseInsensitive);
}
}

MediaRendererConnectionWindow::MediaRendererConnectionWindow(const QString& contentFormat,
                                                             QNetworkAccessManager& nam,
                                                             QWidget* parent)
    : QWidget(parent),
      ui(new Ui::MediaRendererConnectionWindow()),
      m_rendererConnection(0),
      m_nam(nam)
{
    ui->setupUi(this);
    ui->scrollArea->setWidgetResizable(true);

    qDebug() << "Required format to render:" << contentFormat;

    if (isAudio(contentFormat))
    {
#ifdef HAVE_MEDIAPLAYER
        m_rendererConnection = new DefaultRendererConnection(
            DefaultRendererConnection::AudioOnly, ui->scrollAreaWidgetContents);
#else
        qDebug() << "AvTest is compiled without QtAV media player support...";
#endif
    }
    else if (isVideo(contentFormat))
    {
#ifdef HAVE_MEDIAPLAYER
        m_rendererConnection = new DefaultRendererConnection(
            DefaultRendererConnection::AudioVideo, ui->scrollAreaWidgetContents);
#else
        qDebug() << "AvTest is compiled without QtAV media player support...";
#endif
    }
    else if (isImage(contentFormat))
    {
        m_rendererConnection = new RendererConnectionForImagesAndText(
            RendererConnectionForImagesAndText::Images, m_nam, ui->scrollAreaWidgetContents);
    }
    else if (isText(contentFormat))
    {
        m_rendererConnection = new RendererConnectionForImagesAndText(
            RendererConnectionForImagesAndText::Text, m_nam, ui->scrollAreaWidgetContents);
    }
    else if (contentFormat == QLatin1String("*") || contentFormat.isEmpty() ||
             contentFormat == QLatin1String("application/octet-stream"))
    {
#ifdef HAVE_MEDIAPLAYER
        m_rendererConnection = new DefaultRendererConnection(
            DefaultRendererConnection::Unknown, ui->scrollAreaWidgetContents);
#else
        qDebug() << "AvTest is compiled without QtAV media player support...";
#endif
    }
    else
    {
        deleteLater();
        return;
    }

    bool ok = connect(
        m_rendererConnection, SIGNAL(disposed(Herqq::Upnp::Av::HRendererConnection*)),
        this, SLOT(disposed(Herqq::Upnp::Av::HRendererConnection*)));
    Q_ASSERT(ok); Q_UNUSED(ok)
}

MediaRendererConnectionWindow::~MediaRendererConnectionWindow()
{
    if (m_rendererConnection)
    {
        m_rendererConnection->deleteLater();
    }
    delete ui;
}

HRendererConnection* MediaRendererConnectionWindow::rendererConnection() const
{
    return m_rendererConnection;
}

void MediaRendererConnectionWindow::disposed(Herqq::Upnp::Av::HRendererConnection*)
{
    deleteLater();
}

void MediaRendererConnectionWindow::closeEvent(QCloseEvent* e)
{
    e->ignore();
    //deleteLater();
}

void MediaRendererConnectionWindow::resizeEvent(QResizeEvent* e)
{
    m_rendererConnection->resizeEventOccurred(*e);
}
