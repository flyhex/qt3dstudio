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

#include "q3dspresentationitem.h"

#include <QtStudio3D/private/q3dspresentation_p.h>
#include <QtStudio3D/private/viewerqmlstreamproxy_p.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

Q3DSPresentationItem::Q3DSPresentationItem(QObject *parent)
    : Q3DSPresentation(parent)
    , m_subPresentationSettings(nullptr)
{
}

Q3DSPresentationItem::~Q3DSPresentationItem()
{
}

Q3DSSubPresentationSettings *Q3DSPresentationItem::subPresentationSettings() const
{
    return m_subPresentationSettings;
}

QQmlListProperty<QObject> Q3DSPresentationItem::qmlChildren()
{
    return QQmlListProperty<QObject>(this, nullptr, &appendQmlChildren, nullptr, nullptr, nullptr);
}

void Q3DSPresentationItem::appendQmlChildren(QQmlListProperty<QObject> *list, QObject *element)
{
    auto item = qobject_cast<Q3DSPresentationItem *>(list->object);
    if (item) {
        auto scene = qobject_cast<Q3DSSceneElement *>(element);
        if (scene) {
            if (item->registeredElement(scene->elementPath()))
                qWarning() << __FUNCTION__ << "A duplicate SceneElement defined for Presentation.";
            else
                item->registerElement(scene);
        }
        auto studioElement = qobject_cast<Q3DSElement *>(element);
        if (studioElement) {
            if (item->registeredElement(studioElement->elementPath()))
                qWarning() << __FUNCTION__ << "A duplicate Element defined for Presentation.";
            else
                item->registerElement(studioElement);
        }
        auto subPresSettings = qobject_cast<Q3DSSubPresentationSettings *>(element);
        if (subPresSettings) {
            if (item->m_subPresentationSettings) {
                qWarning() << __FUNCTION__
                           << "Duplicate SubPresentationSettings defined for Presentation.";
            } else {
                item->m_subPresentationSettings = subPresSettings;
                item->d_ptr->streamProxy()->setSettings(subPresSettings);
            }
        }
    }
}


QT_END_NAMESPACE
