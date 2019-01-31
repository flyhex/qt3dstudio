/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "Qt3DSCommonPrecompile.h"
#include "CoreUtils.h"
#include "StudioPreferences.h"
#include "StudioClipboard.h"
#include "Pt.h"
#include "StudioUtils.h"
#include "StudioApp.h"
#include "MainFrm.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtCore/qurl.h>

QString StudioUtils::resourcePath()
{
    return QStringLiteral(":/res");
}

QString StudioUtils::resourceImagePath()
{
    return QStringLiteral(":/images/");
}

QString StudioUtils::resourceImageUrl()
{
    return QStringLiteral("qrc:/images/");
}

// Returns the qml import path required for binary installations
QString StudioUtils::qmlImportPath()
{
    QString extraImportPath(QStringLiteral("%1/qml"));
    return extraImportPath.arg(QApplication::applicationDirPath());
}

qreal StudioUtils::devicePixelRatio(QWindow *window)
{
    qreal pixelRatio = 1.0;

    QWindow *w = window ? window
                        : g_StudioApp.m_pMainWnd
                          ? g_StudioApp.m_pMainWnd->windowHandle() : nullptr;

    if (w) {
        QScreen *s = w->screen();
        if (s)
            pixelRatio = s->devicePixelRatio();
    }

    return pixelRatio;
}

// Reads the contents of a text file into QDomDocument
bool StudioUtils::readFileToDomDocument(const QString &filePath, QDomDocument &domDoc)
{
    QFile file(filePath);
    if (!file.open(QFile::Text | QIODevice::ReadOnly)) {
        qWarning() << __FUNCTION__ << file.errorString() << "'" << filePath << "'";
        return false;
    }

    return domDoc.setContent(&file);
}

// Opens a text file for saving and reads its contents into QDomDocument
bool StudioUtils::openDomDocumentSave(QSaveFile &file, QDomDocument &domDoc)
{
    if (!readFileToDomDocument(file.fileName(), domDoc))
        return false;
    if (!file.open(QFile::Text | QIODevice::WriteOnly)) {
        qWarning() << __FUNCTION__ << file.errorString();
        return false;
    }
    return true;
}

// Saves contents of a QDomDocument into a previously opened text file
bool StudioUtils::commitDomDocumentSave(QSaveFile &file, const QDomDocument &domDoc)
{
    // Overwrites entire file
    if (file.resize(0) && file.write(domDoc.toByteArray(4)) != -1 && file.commit())
        return true;

    qWarning() << __FUNCTION__ << file.errorString();
    return false;
}

// Opens text file for saving without reading its contents
bool StudioUtils::openTextSave(QSaveFile &file)
{
    if (!file.open(QFile::Text | QIODevice::WriteOnly)) {
        qWarning() << __FUNCTION__ << file.errorString();
        return false;
    }
    return true;
}
