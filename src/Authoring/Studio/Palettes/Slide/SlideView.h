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

#ifndef SLIDEVIEW_H
#define SLIDEVIEW_H

#include <QtQuickWidgets/qquickwidget.h>
#include <QtCore/qtimer.h>

#include "SlideModel.h"
#include "Qt3DSDMSignals.h"
#include "DispatchListeners.h"

class CClientDataModelBridge;
class CDoc;
class DataInputSelectView;

QT_FORWARD_DECLARE_CLASS(QLabel);

namespace qt3dsdm {
class ISlideSystem;
}

class SlideView : public QQuickWidget,
                  public CPresentationChangeListener,
                  public IDataModelListener
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *currentModel READ currentModel NOTIFY currentModelChanged FINAL)
    Q_PROPERTY(bool showMasterSlide READ showMasterSlide WRITE setShowMasterSlide NOTIFY showMasterSlideChanged FINAL)
    Q_PROPERTY(bool controlled MEMBER m_controlled NOTIFY controlledChanged)
    Q_PROPERTY(QString currController MEMBER m_currentController NOTIFY controlledChanged)
    Q_PROPERTY(QString toolTip MEMBER m_toolTip NOTIFY controlledChanged)
    Q_PROPERTY(Qt::DockWidgetArea dockArea MEMBER m_dockArea NOTIFY dockAreaChanged)
public:
    SlideView(QWidget *parent = nullptr);
    ~SlideView();

    bool showMasterSlide() const;
    void setShowMasterSlide(bool show);
    QAbstractItemModel *currentModel() { return m_CurrentModel; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void onDataInputChange(int handle, int instance, const QString &dataInputName);
    void onDockLocationChange(Qt::DockWidgetArea area);
    void refreshVariants();

    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE void addNewSlide(int row);
    Q_INVOKABLE void removeSlide(int row);
    Q_INVOKABLE void duplicateSlide(int row);
    Q_INVOKABLE void startSlideRearrange(int row);
    Q_INVOKABLE void moveSlide(int from, int to);
    Q_INVOKABLE void finishSlideRearrange(bool commit);
    Q_INVOKABLE void showContextMenu(int x, int y, int row);
    Q_INVOKABLE void showControllerDialog(const QPoint &point);
    Q_INVOKABLE void showVariantsTooltip(int row, const QPoint &point);
    Q_INVOKABLE void hideVariantsTooltip();
    Q_INVOKABLE bool toolTipsEnabled();

    // Presentation Change Listener
    void OnNewPresentation() override;
    void OnClosingPresentation() override;

    // IDataModelListener
    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                            long inInstanceCount) override;

Q_SIGNALS:
    void currentModelChanged();
    void showMasterSlideChanged();
    void controlledChanged();
    void dockAreaChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    // DataModel callbacks
    virtual void OnActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                               const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    virtual void OnNewSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    virtual void OnDeleteSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    virtual void OnSlideRearranged(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inOldIndex,
                                   int inNewIndex);

    void updateDataInputStatus();
    void UpdateSlideViewTitleColor();

private:
    void initialize();
    void clearSlideList();
    void setActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inActiveSlideHandle);
    inline CDoc *GetDoc() const;
    inline CClientDataModelBridge *GetBridge() const;
    inline qt3dsdm::ISlideSystem *GetSlideSystem() const;
    long GetSlideIndex(const qt3dsdm::Qt3DSDMSlideHandle &inSlideHandle) const;
    bool isMaster(const qt3dsdm::Qt3DSDMSlideHandle &inSlideHandle) const;
    void rebuildSlideList(const qt3dsdm::Qt3DSDMSlideHandle &inActiveSlideHandle);
    void onAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void onAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty);

    SlideModel *m_MasterSlideModel = nullptr;
    SlideModel *m_SlidesModel = nullptr;
    SlideModel *m_CurrentModel = nullptr;
    DataInputSelectView *m_dataInputSelector = nullptr;
    QLabel *m_variantsToolTip = nullptr;
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_Connections;
    // We need to remember which slide we were on when we entered the master slide.
    // Then, when the users leave the master slide we can go back to roughly the same
    // state.
    QHash<int, int> m_MasterSlideReturnPointers;

    // the object containing the slides to be inspected.
    qt3dsdm::Qt3DSDMInstanceHandle m_ActiveRoot = 0;
    qt3dsdm::Qt3DSDMSlideHandle m_ActiveSlideHandle; // the active slide handle
    bool m_controlled = false; // Are slides in this slide set controlled by datainput?
    QString m_currentController;
    QString m_toolTip;
    Qt::DockWidgetArea m_dockArea;
    QTimer m_variantRefreshTimer;
};

#endif // SLIDEVIEW_H
