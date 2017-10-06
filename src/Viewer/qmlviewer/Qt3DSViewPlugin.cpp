/****************************************************************************
**
** Copyright (c) 2016 NVIDIA CORPORATION.
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

#include "Qt3DSViewPlugin.h"

#include <QtQml/qqml.h>

#include <QtStudio3D/private/q3dsviewersettings_p.h>

#include "Qt3DSView.h"
#include "q3dspresentationitem.h"
#include "q3dsqmlstream.h"
#include "q3dsqmlsubpresentationsettings.h"
#include "q3dssceneelement.h"

QT_BEGIN_NAMESPACE

//TODO: remove if works with X
/*
#if defined _LINUX || defined linux
extern "C" {
bool InitializeGL();
}
#endif
*/

void Q3DSViewPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("QtStudio3D"));
//TODO: remove if works with X
/*
#if defined _LINUX || defined linux
    // To initialize GL functions for Qt3DS GL bindings
    InitializeGL();
#endif
*/
    // @uri Studio3D
    qmlRegisterType<Q3DSView>(uri, 1, 0, "Studio3D");
    qmlRegisterType<Q3DSViewerSettings>(uri, 1, 0, "ViewerSettings");
    qmlRegisterType<Q3DSPresentationItem>(uri, 1, 0, "Presentation");
    qmlRegisterType<Q3DSSceneElement>(uri, 1, 0, "SceneElement");
    qmlRegisterType<Q3DSElement>(uri, 1, 0, "Element");
    qmlRegisterType<Q3DSQmlStream>(uri, 1, 0, "QmlStream");
    qmlRegisterType<Q3DSSubPresentationSettings>(uri, 1, 0, "SubPresentationSettings");
}

QT_END_NAMESPACE
