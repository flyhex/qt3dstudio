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

class InspectorControlModel;
class QAbstractItemModel;
class CInspectableBase;
class ImageChooserView;
class ImageChooserModel;
class MeshChooserView;
class ObjectBrowserView;
class ObjectListModel;
class FileChooserView;
class TextureChooserView;

class InspectorControlView : public QQuickWidget,
                             public CPresentationChangeListener,
                             public IDataModelListener,
                             public TabNavigable
{
    Q_OBJECT
    Q_PROPERTY(QString titleText READ titleText NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString titleIcon READ titleIcon NOTIFY titleChanged FINAL)
public:
    explicit InspectorControlView(QWidget *parent = nullptr);
    ~InspectorControlView();

    void OnSelectionSet(Q3DStudio::SSelectedValue inValue);
    QAbstractItemModel *inspectorControlModel() const;

    QString titleText() const;
    Q_INVOKABLE QColor titleColor(int instance = 0, int handle = 0) const;
    QString titleIcon() const;

    Q_INVOKABLE void showContextMenu(int x, int y, int handle, int instance);
    Q_INVOKABLE QObject *showImageChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showFilesChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showMeshChooser(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showObjectReference(int handle, int instance, const QPoint &point);
    Q_INVOKABLE QObject *showTextureChooser(int handle, int instance, const QPoint &point);

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

public Q_SLOTS:
    void toggleMasterLink();

protected:
    QSize sizeHint() const override;

private:
    void setInspectable(CInspectableBase *inInspectable);
    void updateInspectable(CInspectableBase *inInspectable);
    void initialize();
    void onFilesChanged(const Q3DStudio::TFileModificationList &inFileModificationList);
    void OnNewPresentation() override;
    void OnClosingPresentation() override;
    void OnLoadedSubPresentation() override;
    void OnTimeChanged();
    void filterMaterials(std::vector<Q3DStudio::CFilePath> &materials);
    void setPropertyValueFromFilename(long instance, int handle, const QString &name);
    void showBrowser(QQuickWidget *browser, const QPoint &point);

    std::shared_ptr<qt3dsdm::ISignalConnection> m_selectionChangedConnection;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_timeChanged;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_DirectoryConnection;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_PropertyChangeConnection;
    QColor m_backgroundColor;
    InspectorControlModel *m_inspectorControlModel = nullptr;
    CInspectableBase *m_inspectableBase = nullptr;
    QPointer<ImageChooserView> m_imageChooserView;
    QPointer<MeshChooserView> m_meshChooserView;
    QPointer<FileChooserView> m_fileChooserView;
    QPointer<TextureChooserView> m_textureChooserView;
    QPointer<ObjectBrowserView> m_objectReferenceView;
    QPointer<ObjectListModel> m_objectReferenceModel;
    std::vector<Q3DStudio::CFilePath> m_fileList;
    MouseHelper m_mouseHelper;

    int m_instance;
    int m_handle;
};

#endif // INSPECTORCONTROLVIEW_H
