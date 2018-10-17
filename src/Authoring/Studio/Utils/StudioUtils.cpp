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

qreal StudioUtils::devicePixelRatio()
{
    qreal pixelRatio = 1.0;

    const QWindowList list = QGuiApplication::topLevelWindows();
    if (list.size() > 0)
        pixelRatio = list[0]->devicePixelRatio();

    return pixelRatio;
}
