/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "MaterialRefView.h"
#include "StudioApp.h"
#include "StudioPreferences.h"
#include "Doc.h"
#include "Core.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "IObjectReferenceHelper.h"
#include "IDocumentEditor.h"

#include <QtCore/qtimer.h>

MaterialRefView::MaterialRefView(QWidget *parent)
: QListWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QListWidget::Fixed);
}

/**
 * Gather and display the currently used standard and custom material list
 *
 * @param instance The instance that owns the property handle
 * @param handle The property handle this materials list is for
 *
 * @return necessary size for the dialog
 */
QSize MaterialRefView::refreshMaterials(int instance, int handle)
{
    clear(); // clear old material list

    m_instance = instance;
    m_handle = handle;
    qt3dsdm::Qt3DSDMInstanceHandle refInstance = getRefInstance();

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
    auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    static const QPixmap pixMaterialNormal = QPixmap(":/images/Objects-Material-Normal.png");

    QVector<qt3dsdm::Qt3DSDMInstanceHandle> mats;
    doc->getSceneMaterials(doc->GetSceneInstance(), mats);

    // add freshly collected material list
    for (auto matInstance : qAsConst(mats)) {
        qt3dsdm::SValue v;
        propertySystem->GetInstancePropertyValue(matInstance,
                                bridge->GetObjectDefinitions().m_Named.m_NameProp, v);
        QString matName = qt3dsdm::get<QString>(v);

        // get the material's object name (parent)
        qt3dsdm::Qt3DSDMInstanceHandle parentInstance = bridge->GetParentInstance(matInstance);
        qt3dsdm::SValue vParent;
        propertySystem->GetInstancePropertyValue(parentInstance,
                                bridge->GetObjectDefinitions().m_Named.m_NameProp, vParent);
        QString objName = qt3dsdm::get<QString>(vParent);
        matName.append(QLatin1String(" (") + objName + QLatin1String(")"));

        QListWidgetItem *matItem = new QListWidgetItem(this);
        matItem->setData(Qt::DisplayRole, matName);
        matItem->setData(Qt::DecorationRole, pixMaterialNormal);
        matItem->setData(Qt::UserRole, QVariant(matInstance));

        if (matInstance == refInstance)
            setCurrentItem(matItem);
    }

    if (count() == 0) {
        // Show an unselectable dummy item
        static const QPixmap pixWarning = QPixmap(":/images/warning.png");
        QListWidgetItem *matItem = new QListWidgetItem(this);
        matItem->setData(Qt::DisplayRole, tr("No animatable materials found"));
        matItem->setData(Qt::DecorationRole, pixWarning);
        matItem->setData(Qt::UserRole, -1);
        setSelectionMode(QAbstractItemView::NoSelection);
    } else {
        setSelectionMode(QAbstractItemView::SingleSelection);
    }

    QSize widgetSize(CStudioPreferences::valueWidth(),
                     qMin(10, count()) * CStudioPreferences::controlBaseHeight());

    return widgetSize;
}

void MaterialRefView::updateSelection()
{
    int refInstance = getRefInstance();
    for (int i = 0, itemCount = count(); i < itemCount; ++i) {
        int matInstance = item(i)->data(Qt::UserRole).toInt();
        if (matInstance == refInstance) {
            setCurrentRow(i);
            break;
        }
    }
}

bool MaterialRefView::isFocused() const
{
    return hasFocus();
}

int MaterialRefView::getRefInstance() const
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();

    qt3dsdm::SValue value;
    propertySystem->GetInstancePropertyValue(m_instance, m_handle, value);
    return doc->GetDataModelObjectReferenceHelper()->Resolve(value, m_instance);
}

void MaterialRefView::focusInEvent(QFocusEvent *event)
{
    QAbstractItemView::focusInEvent(event);
    emit focusChanged();
}

void MaterialRefView::focusOutEvent(QFocusEvent *event)
{
    QAbstractItemView::focusOutEvent(event);
    emit focusChanged();
    QTimer::singleShot(0, this, &QAbstractItemView::close);
}
