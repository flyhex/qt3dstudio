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

#include <QQuickWidget>

#include "DispatchListeners.h"
#include "SlideModel.h"
#include "DataInputSelectView.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSignals.h"
#include "DispatchListeners.h"
#include "Dispatch.h"
#include <unordered_map>
class CClientDataModelBridge;
class CDoc;

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
public:
    SlideView(QWidget *parent = nullptr);
    ~SlideView();

    bool showMasterSlide() const;
    void setShowMasterSlide(bool show);
    QAbstractItemModel *currentModel() { return m_CurrentModel; }
    QSize sizeHint() const override;
    void onDataInputChange(int handle, int instance, const QString &dataInputName);

    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE void addNewSlide(int row);
    Q_INVOKABLE void removeSlide(int row);
    Q_INVOKABLE void duplicateSlide(int row);
    Q_INVOKABLE void startSlideRearrange(int row);
    Q_INVOKABLE void moveSlide(int from, int to);
    Q_INVOKABLE void finishSlideRearrange(bool commit);
    Q_INVOKABLE void showContextMenu(int x, int y, int row);
    Q_INVOKABLE void showControllerDialog(const QPoint &point);
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


protected:
    // DataModel callbacks
    virtual void OnActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                               const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    virtual void OnNewSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    virtual void OnDeleteSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    virtual void OnSlideRearranged(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inOldIndex,
                                   int inNewIndex);

    void updateDataInputStatus(bool isViaDispatch);
    void UpdateSlideViewTitleColor();

private:
    void initialize();
    void clearSlideList();
    void setActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inActiveSlideHandle);
    inline CDoc *GetDoc();
    inline CClientDataModelBridge *GetBridge();
    inline qt3dsdm::ISlideSystem *GetSlideSystem();
    long GetSlideIndex(const qt3dsdm::Qt3DSDMSlideHandle &inSlideHandle);
    bool isMaster(const qt3dsdm::Qt3DSDMSlideHandle &inSlideHandle);
    void rebuildSlideList(const qt3dsdm::Qt3DSDMSlideHandle &inActiveSlideHandle);

    SlideModel *m_CurrentModel = nullptr;
    SlideModel *m_MasterSlideModel = nullptr;
    SlideModel *m_SlidesModel = nullptr;
    DataInputSelectView *m_dataInputSelector = nullptr;
    QColor m_BaseColor = QColor::fromRgb(75, 75, 75);
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>>
        m_Connections; /// connections to the DataModel
    typedef std::unordered_map<int, int> TIntIntMap;
    // We need to remember which slide we were on when we entered the master slide.
    // Then, when the users leave the master slide we can go back to roughly the same
    // state.
    TIntIntMap m_MasterSlideReturnPointers;

    qt3dsdm::Qt3DSDMInstanceHandle m_ActiveRoot; ///< the object containing the slides to be inspected.
    qt3dsdm::Qt3DSDMSlideHandle m_ActiveSlideHandle; ///< the active slide handle
    bool m_controlled; // Are slides in this slide set controlled by datainput?
    QString m_currentController;
    QString m_toolTip;
};

#endif // SLIDEVIEW_H
