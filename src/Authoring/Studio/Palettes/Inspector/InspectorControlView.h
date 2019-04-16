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

#ifndef INSPECTORCONTROLVIEW_H
#define INSPECTORCONTROLVIEW_H

#include <QtQuickWidgets/qquickwidget.h>
#include <QtCore/qpointer.h>
#include "DispatchListeners.h"
#include "Dispatch.h"
#include "Qt3DSFileTools.h"
#include "TabOrderHandler.h"
#include "MouseHelper.h"
#include "QmlUtils.h"
#include "DataInputSelectView.h"

class InspectorControlModel;
class VariantsGroupModel;
class CInspectableBase;
class ImageChooserView;
class DataInputSelectView;
class ImageChooserModel;
class MeshChooserView;
class ObjectBrowserView;
class ObjectListModel;
class FileChooserView;
class TextureChooserView;
class MaterialRefView;

QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)

class InspectorControlView : public QQuickWidget,
                             public CPresentationChangeListener,
                             public IDataModelListener,
                             public TabNavigable
{
    Q_OBJECT
    Q_PROPERTY(QString titleText READ titleText NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString titleIcon READ titleIcon NOTIFY titleChanged FINAL)

public:
    explicit InspectorControlView(const QSize &preferredSize, QWidget *parent = nullptr);
    ~InspectorControlView() override;

    void OnSelectionSet(Q3DStudio::SSelectedValue inValue);
    QAbstractItemModel *inspectorControlModel() const;

    QString titleText() const;
    QString titleIcon() const;
    VariantsGroupModel *variantsModel() const { return m_variantsGroupModel; }

    Q_INVOKABLE QColor titleColor(int instance = 0, int handle = 0) const;
    Q_INVOKABLE QColor showColorDialog(const QColor &color, int instance = 0, int handle = 0);
    Q_INVOKABLE void showContextMenu(int x, int y, int handle, int instance);
    Q_INVOKABLE void showTagContextMenu(int x, int y, const QString &group, const QString &tag);
    Q_INVOKABLE void showDataInputChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE void showGroupContextMenu(int x, int y, const QString &group);
    Q_INVOKABLE QObject *showImageChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showFilesChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showMeshChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showObjectReference(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showMaterialReference(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showTextureChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE bool toolTipsEnabled();
    Q_INVOKABLE bool isRefMaterial(int instance) const;
    Q_INVOKABLE bool isEditable(int handle) const;
    Q_INVOKABLE QString convertPathToProjectRoot(const QString &presentationPath);
    Q_INVOKABLE QString noneString() const;

    // IDataModelListener
    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                            long inInstanceCount) override;

Q_SIGNALS:
    void titleChanged();
    void controlsChanged();
    void imageSelected(const QString &name);
    void dialogCurrentColorChanged(const QColor &newColor);

public Q_SLOTS:
    void toggleMasterLink();

protected:
    QSize sizeHint() const override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setInspectable(CInspectableBase *inInspectable);
    void initialize();
    void onFilesChanged(const Q3DStudio::TFileModificationList &inFileModificationList);
    void OnNewPresentation() override;
    void OnClosingPresentation() override;
    void filterMaterials(std::vector<Q3DStudio::CFilePath> &materials);
    void filterMatDatas(std::vector<Q3DStudio::CFilePath> &matDatas);
    void setPropertyValueFromFilename(long instance, int handle, const QString &name);
    CInspectableBase *createInspectableFromSelectable(Q3DStudio::SSelectedValue selectable);
    bool canLinkProperty(int instance, int handle) const;
    bool canOpenInInspector(int instance, int handle) const;
    void openInInspector();
    void onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void onChildAdded(int inChild);
    void onChildRemoved();

    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_connections;
    QColor m_backgroundColor;
    VariantsGroupModel *m_variantsGroupModel = nullptr;
    InspectorControlModel *m_inspectorControlModel = nullptr;
    CInspectableBase *m_inspectableBase = nullptr;
    QPointer<ImageChooserView> m_imageChooserView;
    QPointer<MeshChooserView> m_meshChooserView;
    QPointer<FileChooserView> m_fileChooserView;
    QPointer<TextureChooserView> m_textureChooserView;
    QPointer<ObjectBrowserView> m_objectReferenceView;
    QPointer<MaterialRefView> m_matRefListWidget;
    QPointer<ObjectListModel> m_objectReferenceModel;
    QPointer<DataInputSelectView> m_dataInputChooserView;
    std::vector<Q3DStudio::CFilePath> m_fileList;
    MouseHelper m_mouseHelper;
    QmlUtils m_qmlUtils;

    int m_contextMenuInstance = 0;
    int m_contextMenuHandle = 0;

    QSize m_preferredSize;
    QColor m_currentColor;

    class ActiveBrowserData
    {
    public:
        void setData(QWidget *browser, int handle, int instance)
        {
            m_browser = browser;
            m_handle = handle;
            m_instance = instance;
        }
        void clear()
        {
            if (isActive())
                m_browser->close();
            m_browser.clear();
            m_handle = -1;
            m_instance = -1;
        }
        bool isActive() const
        {
            return !m_browser.isNull() && m_browser->isVisible();
        }

        QPointer<QWidget> m_browser = nullptr;
        int m_handle = -1;
        int m_instance = -1;
    };

    ActiveBrowserData m_activeBrowser;
};

#endif // INSPECTORCONTROLVIEW_H
