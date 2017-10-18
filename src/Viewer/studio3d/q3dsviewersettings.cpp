/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "q3dsviewersettings_p.h"
#include "Qt3DSViewerApp.h"
#include "q3dscommandqueue_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

Q3DSViewerSettings::Q3DSViewerSettings(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSViewerSettingsPrivate(this))
{
}

Q3DSViewerSettings::~Q3DSViewerSettings()
{
}

QColor Q3DSViewerSettings::matteColor() const
{
    return d_ptr->m_matteColor;
}

bool Q3DSViewerSettings::isShowRenderStats() const
{
    return d_ptr->m_showRenderStats;
}

Q3DSViewerSettings::ShadeMode Q3DSViewerSettings::shadeMode() const
{
    return d_ptr->m_shadeMode;
}

Q3DSViewerSettings::ScaleMode Q3DSViewerSettings::scaleMode() const
{
    return d_ptr->m_scaleMode;
}

void Q3DSViewerSettings::save(const QString &group, const QString &organization,
                                      const QString &application)
{
    d_ptr->save(group, organization, application);
}

void Q3DSViewerSettings::load(const QString &group, const QString &organization,
                                      const QString &application)
{
    d_ptr->load(group, organization, application);
}

void Q3DSViewerSettings::setMatteColor(const QColor &color)
{
    if (d_ptr->m_matteColor != color) {
        d_ptr->setMatteColor(color);
        Q_EMIT matteColorChanged(color);
    }
}

void Q3DSViewerSettings::setShowRenderStats(bool show)
{
    if (d_ptr->m_showRenderStats != show) {
        d_ptr->setShowRenderStats(show);
        Q_EMIT showRenderStatsChanged(show);
    }
}

void Q3DSViewerSettings::setShadeMode(Q3DSViewerSettings::ShadeMode mode)
{
    if (d_ptr->m_shadeMode != mode) {
        d_ptr->setShadeMode(mode);
        Q_EMIT shadeModeChanged(mode);
    }

}

void Q3DSViewerSettings::setScaleMode(Q3DSViewerSettings::ScaleMode mode)
{
    if (d_ptr->m_scaleMode != mode) {
        d_ptr->setScaleMode(mode);
        Q_EMIT scaleModeChanged(mode);
    }
}

Q3DSViewerSettingsPrivate::Q3DSViewerSettingsPrivate(Q3DSViewerSettings *q)
    : QObject(q)
    , q_ptr(q)
    , m_viewerApp(nullptr)
    , m_commandQueue(nullptr)
    , m_matteColor(Qt::black)
    , m_showRenderStats(false)
    , m_shadeMode(Q3DSViewerSettings::ShadeModeShaded)
    , m_scaleMode(Q3DSViewerSettings::ScaleModeCenter)
    , m_savedSettings(nullptr)
{
}

Q3DSViewerSettingsPrivate::~Q3DSViewerSettingsPrivate()
{
}

void Q3DSViewerSettingsPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;
    if (m_viewerApp) {
        setMatteColor(m_matteColor);
        setShowRenderStats(m_showRenderStats);
        setShadeMode(m_shadeMode);
        setScaleMode(m_scaleMode);
    }
}

void Q3DSViewerSettingsPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;
    if (m_commandQueue) {
        setMatteColor(m_matteColor);
        setShowRenderStats(m_showRenderStats);
        setShadeMode(m_shadeMode);
        setScaleMode(m_scaleMode);
    }
}

void Q3DSViewerSettingsPrivate::save(const QString &group, const QString &organization,
                                             const QString &application)
{
    initSettingsStore(group, organization, application);

    m_savedSettings->setValue(QStringLiteral("matteColor"), m_matteColor);
    m_savedSettings->setValue(QStringLiteral("showRenderStats"), m_showRenderStats);
    m_savedSettings->setValue(QStringLiteral("shadeMode"), m_shadeMode);
    m_savedSettings->setValue(QStringLiteral("scaleMode"), m_scaleMode);
}

void Q3DSViewerSettingsPrivate::load(const QString &group, const QString &organization,
                                             const QString &application)
{
    initSettingsStore(group, organization, application);

    q_ptr->setMatteColor(m_savedSettings->value(QStringLiteral("matteColor")).value<QColor>());
    q_ptr->setShowRenderStats(m_savedSettings->value(QStringLiteral("showRenderStats")).toBool());
    q_ptr->setShadeMode(Q3DSViewerSettings::ShadeMode(
                            m_savedSettings->value(QStringLiteral("shadeMode")).toInt()));
    q_ptr->setScaleMode(Q3DSViewerSettings::ScaleMode(
                            m_savedSettings->value(QStringLiteral("scaleMode")).toInt()));
}

void Q3DSViewerSettingsPrivate::setMatteColor(const QColor &color)
{
    m_matteColor = color;
    if (m_viewerApp) {
        m_viewerApp->setMatteColor(color);
    } else if (m_commandQueue) {
        m_commandQueue->m_matteColor = color;
        m_commandQueue->m_matteColorChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::setShowRenderStats(bool show)
{
    m_showRenderStats = show;
    if (m_viewerApp) {
        m_viewerApp->setShowOnScreenStats(show);
    } else if (m_commandQueue) {
        m_commandQueue->m_showRenderStats = show;
        m_commandQueue->m_showRenderStatsChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::setShadeMode(Q3DSViewerSettings::ShadeMode mode)
{
    m_shadeMode = mode;
    if (m_viewerApp) {
        if (mode == Q3DSViewerSettings::ShadeModeShaded)
            m_viewerApp->SetShadeMode(Q3DSViewer::ViewerShadeModes::Shaded);
        else
            m_viewerApp->SetShadeMode(Q3DSViewer::ViewerShadeModes::ShadedWireframe);
    } else if (m_commandQueue) {
        m_commandQueue->m_shadeMode = mode;
        m_commandQueue->m_shadeModeChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::setScaleMode(Q3DSViewerSettings::ScaleMode mode)
{
    m_scaleMode = mode;
    if (m_viewerApp) {
        if (mode == Q3DSViewerSettings::ScaleModeFit)
            m_viewerApp->SetScaleMode(Q3DSViewer::ViewerScaleModes::ScaleToFit);
        else if (mode == Q3DSViewerSettings::ScaleModeFill)
            m_viewerApp->SetScaleMode(Q3DSViewer::ViewerScaleModes::ScaleToFill);
        else
            m_viewerApp->SetScaleMode(Q3DSViewer::ViewerScaleModes::ExactSize);
    } else if (m_commandQueue) {
        m_commandQueue->m_scaleMode = mode;
        m_commandQueue->m_scaleModeChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::initSettingsStore(const QString &group, const QString &organization,
                                                  const QString &application)
{
    if (!m_savedSettings) {
        QString org = organization.isEmpty() ? QCoreApplication::instance()->organizationName()
                                             : organization;
        QString app = application.isEmpty() ? QCoreApplication::instance()->applicationName()
                                            : application;

        m_savedSettings = new QSettings(org, app, this);
        m_savedSettings->beginGroup(group);
    }
}

QT_END_NAMESPACE
