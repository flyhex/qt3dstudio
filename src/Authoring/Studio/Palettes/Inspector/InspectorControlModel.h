/****************************************************************************
**
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

#ifndef INSPECTORCONTROLMODEL_H
#define INSPECTORCONTROLMODEL_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qvector.h>

#include "UICDMValue.h"
#include "UICDMMetaDataValue.h"
#include "UICDMMetaDataTypes.h"
#include "UICFileTools.h"

#include "IDocumentEditor.h"

class CInspectableBase;
class CUICDMInspectable;
class SGuideInspectableImpl;

namespace qt3dsdm {
class ISignalConnection;
typedef std::shared_ptr<ISignalConnection> TSignalConnectionPtr;
}

namespace Q3DStudio
{
class CUICDMInspectorRow;
}

class InspectorControlBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qt3dsdm::DataModelDataType::Value dataType MEMBER m_dataType CONSTANT)
    Q_PROPERTY(qt3dsdm::AdditionalMetaDataType::Value propertyType MEMBER m_propertyType CONSTANT)
    Q_PROPERTY(QVariant value MEMBER m_value NOTIFY valueChanged)
    Q_PROPERTY(QVariant values MEMBER m_values NOTIFY valuesChanged)
    Q_PROPERTY(QString title MEMBER m_title CONSTANT)
    Q_PROPERTY(QString toolTip MEMBER m_tooltip CONSTANT)
    Q_PROPERTY(int instance MEMBER m_instance CONSTANT)
    Q_PROPERTY(int handle MEMBER m_property CONSTANT)

    Q_PROPERTY(bool animatable MEMBER m_animatable CONSTANT)
    Q_PROPERTY(bool animated MEMBER m_animated NOTIFY animatedChanged)

public:
    virtual ~InspectorControlBase();

Q_SIGNALS:
    void valueChanged();
    void valuesChanged();
    void animatedChanged();

public:
    qt3dsdm::DataModelDataType::Value m_dataType;
    qt3dsdm::AdditionalMetaDataType::Value m_propertyType;
    QVariant m_value;
    QVariant m_values;
    QString m_title;
    QString m_tooltip;

    qt3dsdm::Qt3DSDMInstanceHandle m_instance;
    qt3dsdm::Qt3DSDMPropertyHandle m_property;

    bool m_animatable  = false;
    bool m_animated = false;

    std::vector<qt3dsdm::TSignalConnectionPtr> m_connections;
};

class InspectorControlModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit InspectorControlModel(QObject *parent);
    ~InspectorControlModel() = default;

    enum Roles {
        GroupValuesRole = Qt::UserRole + 1,
        GroupTitleRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    void setInspectable(CInspectableBase *inInspectable);
    void setMaterials(std::vector<Q3DStudio::CFilePath> &materials);
    void refreshRenderables();
    void refresh();

    QVariant getPropertyValue(long instance, int handle);

    Q_INVOKABLE void setMaterialTypeValue(long instance, int handle, const QVariant &value);
    Q_INVOKABLE void setRenderableValue(long instance, int handle, const QVariant &value);
    Q_INVOKABLE void setPropertyValue(long instance, int handle, const QVariant &value, bool commit = true);
    Q_INVOKABLE void setSlideSelection(long instance, int handle, int index,
                                       const QStringList &list);
    Q_INVOKABLE void setPropertyAnimated(long instance, int handle, bool animated);

private:
    void onSlideRearranged(const qt3dsdm::CUICDMSlideHandle &inMaster, int inOldIndex,
                           int inNewIndex);


    struct GroupInspectorControl {
        QString groupTitle;
        QVariantList controlElements;

        ~GroupInspectorControl() {
            //for (auto element : controlElements)
            //    element.value<QObject *>()->deleteLater();
        }
    };

    mutable QVector<GroupInspectorControl> m_groupElements;
    CInspectableBase *m_inspectableBase = nullptr;

    struct MaterialEntry
    {
        QString m_name;
        QString m_relativePath;
    };

    std::vector<MaterialEntry> m_materials;

    Q3DStudio::CUpdateableDocumentEditor m_UpdatableEditor;

    QPair<long, int> m_modifiedProperty;

    void updatePropertyValue(InspectorControlBase *element) const;
    void rebuildTree();
    void refreshTree();
    void notifyInstancePropertyValue(qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void updateAnimateToggleState(InspectorControlBase* inItem);

    std::shared_ptr<qt3dsdm::ISignalConnection> m_notifier;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_slideNotifier;

    InspectorControlBase *createMaterialItem(CUICDMInspectable *inspectable, int groupIndex);
    InspectorControlBase *createItem(CUICDMInspectable *inspectable,
                                     Q3DStudio::CUICDMInspectorRow *row, int groupIndex);
    InspectorControlBase *createItem(CUICDMInspectable *inspectable,
                                     const qt3dsdm::SMetaDataPropertyInfo &metaProperty,
                                     int groupIndex);

    qt3dsdm::SValue currentPropertyValue(long instance, int handle);

    QVector<GroupInspectorControl> computeTree(CInspectableBase *inspectBase);
    bool isTreeRebuildRequired(CInspectableBase *inspectBase) const;

    GroupInspectorControl computeGroup(CInspectableBase* inspectBase, int theIndex);
    bool isGroupRebuildRequired(CInspectableBase *inspectable, int theIndex) const;

};

#endif // INSPECTORCONTROLMODEL_H
