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

#include "Qt3DSDMValue.h"
#include "Qt3DSDMMetaDataValue.h"
#include "Qt3DSDMMetaDataTypes.h"
#include "IDocumentEditor.h"

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qvector.h>

class CInspectableBase;
class Qt3DSDMInspectable;
class GuideInspectable;
class VariantsGroupModel;
class CClientDataModelBridge;

namespace qt3dsdm {
class ISignalConnection;
typedef std::shared_ptr<ISignalConnection> TSignalConnectionPtr;
}

namespace Q3DStudio
{
class Qt3DSDMInspectorRow;
}

class InspectorControlBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qt3dsdm::DataModelDataType::Value dataType MEMBER m_dataType CONSTANT)
    Q_PROPERTY(qt3dsdm::AdditionalMetaDataType::Value propertyType MEMBER m_propertyType CONSTANT)
    Q_PROPERTY(QVariant value MEMBER m_value NOTIFY valueChanged)
    Q_PROPERTY(QVariant values MEMBER m_values NOTIFY valuesChanged)
    Q_PROPERTY(QString title MEMBER m_title CONSTANT)
    Q_PROPERTY(QString toolTip MEMBER m_tooltip NOTIFY tooltipChanged)
    Q_PROPERTY(int instance MEMBER m_instance CONSTANT)
    Q_PROPERTY(int handle MEMBER m_property CONSTANT)

    Q_PROPERTY(bool enabled MEMBER m_enabled CONSTANT)
    Q_PROPERTY(bool animatable MEMBER m_animatable CONSTANT)
    Q_PROPERTY(bool animated MEMBER m_animated NOTIFY animatedChanged)
    Q_PROPERTY(bool controlled MEMBER m_controlled NOTIFY controlledChanged)
    Q_PROPERTY(bool controllable MEMBER m_controllable CONSTANT)
    Q_PROPERTY(QString controller MEMBER m_controller NOTIFY controlledChanged)

public:
    virtual ~InspectorControlBase();

Q_SIGNALS:
    void valueChanged();
    void valuesChanged();
    void animatedChanged();
    void controlledChanged();
    void tooltipChanged();

public:
    qt3dsdm::DataModelDataType::Value m_dataType;
    qt3dsdm::AdditionalMetaDataType::Value m_propertyType;
    qt3dsdm::SMetaDataPropertyInfo m_metaProperty;
    QVariant m_value;
    QVariant m_values;
    QString m_title;
    QString m_tooltip;

    qt3dsdm::Qt3DSDMInstanceHandle m_instance;
    qt3dsdm::Qt3DSDMPropertyHandle m_property;

    bool m_enabled = true;
    bool m_animatable  = false;
    bool m_animated = false;
    bool m_controlled = false;
    bool m_controllable = false;
    QString m_controller;
    std::vector<qt3dsdm::TSignalConnectionPtr> m_connections;
};

class InspectorControlModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit InspectorControlModel(VariantsGroupModel *variantsModel, QObject *parent);
    ~InspectorControlModel() override = default;

    enum Roles {
        GroupValuesRole = Qt::UserRole + 1,
        GroupTitleRole,
        GroupInfoRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    void setInspectable(CInspectableBase *inInspectable);
    CInspectableBase *inspectable() const;
    void setMaterials(std::vector<Q3DStudio::CFilePath> &materials);
    void setMatDatas(const std::vector<Q3DStudio::CFilePath> &matdatas);
    void updateFontValues(InspectorControlBase *element) const;
    void refreshRenderables();
    void refresh();
    void saveIfMaterial(qt3dsdm::Qt3DSDMInstanceHandle instance);

    bool hasInstanceProperty(long instance, int handle);

    qt3dsdm::SValue currentPropertyValue(long instance, int handle) const;
    QString currentControllerValue(long instance, int handle) const;
    void setPropertyControllerInstance(long instance,int handle,
                                       Q3DStudio::CString controllerInstance,
                                       bool controlled);
    void notifyPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                               qt3dsdm::Qt3DSDMPropertyHandle inProperty);

    Q_INVOKABLE void setMaterialTypeValue(long instance, int handle, const QVariant &value);
    Q_INVOKABLE void setShaderValue(long instance, int handle, const QVariant &value);
    Q_INVOKABLE void setMatDataValue(long instance, int handle, const QVariant &value);
    Q_INVOKABLE void setRenderableValue(long instance, int handle, const QVariant &value);
    Q_INVOKABLE void setPropertyValue(long instance, int handle, const QVariant &value, bool commit = true);
    Q_INVOKABLE void setSlideSelection(long instance, int handle, int index,
                                       const QStringList &list);
    Q_INVOKABLE void setPropertyAnimated(long instance, int handle, bool animated);
    Q_INVOKABLE void setPropertyControlled(long instance, int property);
    Q_INVOKABLE bool isLayer(long instance) const;
    Q_INVOKABLE QString renderableId(const QString &filePath) const;
    Q_INVOKABLE bool isMaterial() const;
    Q_INVOKABLE bool isDefaultMaterial() const;
    Q_INVOKABLE void addMaterial();
    Q_INVOKABLE void duplicateMaterial();
    Q_INVOKABLE bool isGroupCollapsed(int groupIdx) const;
    Q_INVOKABLE void updateGroupCollapseState(int groupIdx, bool state);

private:
    void onSlideRearranged(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inOldIndex,
                           int inNewIndex);


    struct GroupInspectorControl {
        QString groupTitle;
        QVariantList controlElements;
        QString groupInfo;

        ~GroupInspectorControl() {
        }
    };

    QVector<GroupInspectorControl> m_groupElements;
    CInspectableBase *m_inspectableBase = nullptr;
    GuideInspectable *m_guideInspectable = nullptr;

    struct MaterialEntry
    {
        QString m_name;
        QString m_relativePath;
    };

    struct MaterialDataEntry
    {
        QString m_name;
        QString m_relativePath;
        QMap<QString, QString> m_values;
        QMap<QString, QMap<QString, QString>> m_textureValues;
    };

    std::vector<MaterialEntry> m_materials;
    std::vector<MaterialDataEntry> m_matDatas;
    std::vector<Q3DStudio::CFilePath> m_cachedMatDatas;
    qt3dsdm::Qt3DSDMInstanceHandle m_refMaterial;

    Q3DStudio::CUpdateableDocumentEditor m_UpdatableEditor;

    QPair<long, int> m_modifiedProperty;

    qt3dsdm::SValue m_previouslyCommittedValue;

    QHash<int, QHash<int, bool> > m_collapseMap;

    QString getBasicMaterialString() const;
    QString getAnimatableMaterialString() const;
    QString getReferencedMaterialString() const;
    QString getStandardMaterialString() const;
    QString getDefaultMaterialString() const;
    bool isInsideMaterialContainer() const;
    bool isInsideMaterialContainer(CInspectableBase *inspectable) const;
    bool isAnimatableMaterial() const;
    bool isAnimatableMaterial(CInspectableBase *inspectable) const;
    bool isBasicMaterial() const;
    bool isBasicMaterial(CInspectableBase *inspectable) const;
    void updateMaterialValues(const QStringList &values, int elementIndex,
                              bool updatingShaders = false);
    void updateShaderValues();
    void updateMatDataValues();
    void updatePropertyValue(InspectorControlBase *element) const;
    void rebuildTree();
    void refreshTree();
    void updateAnimateToggleState(InspectorControlBase *inItem);
    void updateControlledToggleState(InspectorControlBase *inItem) const;

    QStringList materialTypeValues() const;
    QStringList shaderValues() const;
    QStringList matDataValues() const;
    InspectorControlBase *createMaterialTypeItem(Qt3DSDMInspectable *inspectable, int groupIndex);
    InspectorControlBase *createShaderItem(Qt3DSDMInspectable *inspectable, int groupIndex);
    InspectorControlBase *createMatDataItem(Qt3DSDMInspectable *inspectable, int groupIndex);
    InspectorControlBase *createItem(Qt3DSDMInspectable *inspectable,
                                     Q3DStudio::Qt3DSDMInspectorRow *row, int groupIndex);
    InspectorControlBase *createItem(Qt3DSDMInspectable *inspectable,
                                     const qt3dsdm::SMetaDataPropertyInfo &metaProperty,
                                     int groupIndex);

    QVector<GroupInspectorControl> computeTree(CInspectableBase *inspectBase);
    bool isTreeRebuildRequired(CInspectableBase *inspectBase);

    GroupInspectorControl computeGroup(CInspectableBase* inspectBase,
                                       int theIndex, bool disableAnimation = false,
                                       bool isReference = false);
    bool isGroupRebuildRequired(CInspectableBase *inspectable, int theIndex) const;

    CClientDataModelBridge *getBridge() const;

    static int handleToGuidePropIndex(int handle) { return handle - 1; }

    VariantsGroupModel *m_variantsModel = nullptr;
};

#endif // INSPECTORCONTROLMODEL_H
