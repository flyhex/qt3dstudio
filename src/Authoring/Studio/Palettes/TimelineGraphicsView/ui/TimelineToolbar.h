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

#ifndef TIMELINETOOLBAR_H
#define TIMELINETOOLBAR_H

#include "SelectedValueImpl.h"
#include "Qt3DSDMSignals.h"
#include "DispatchListeners.h"
#include "Dispatch.h"
#include "DataInputSelectView.h"
#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qlabel.h>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QSlider)

class TimelineToolbar : public QToolBar,
                        public IDataModelListener
{
    Q_OBJECT

signals:
    void newLayerTriggered();
    void deleteLayerTriggered();
    void gotoTimeTriggered();
    void firstFrameTriggered();
    void stopTriggered();
    void playTriggered();
    void controllerChanged(const QString &controller);
    void lastFrameTriggered();
    void timelineScaleChanged(int scale);
    void setDurationTriggered();
    void showRowTextsToggled(bool toggled);
    void variantsFilterToggled(bool toggled);

public:
    TimelineToolbar();
    virtual ~TimelineToolbar() override;
    void setTime(long totalMillis);
    QString getCurrentController() const;
    void setNewLayerEnabled(bool enable);
    QAction *actionShowRowTexts() const;
    bool isVariantsFilterOn() const;

    // IDataModelListener
    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                            long inInstanceCount) override;

public Q_SLOTS:
    void updatePlayButtonState(bool started);
    void onZoomInButtonClicked();
    void onZoomOutButtonClicked();

private Q_SLOTS:
    void onPlayButtonClicked();
    void onZoomLevelChanged(int scale);
    void onDiButtonClicked();

private:
    void addSpacing(int width);
    void onSelectionChange(Q3DStudio::SSelectedValue inNewSelectable);
    void onDataInputChange(int handle, int instance, const QString &dataInputName);
    void showDataInputChooser(const QPoint &point);
    void updateDataInputStatus();
    void updateTimelineTitleColor(bool controlled);

    QPushButton *m_timeLabel = nullptr;
    QLabel *m_diLabel = nullptr;
    QAction *m_actionDeleteRow = nullptr;
    QAction *m_actionPlayStop = nullptr;
    QAction *m_actionZoomIn = nullptr;
    QAction *m_actionZoomOut = nullptr;
    QAction *m_actionDataInput = nullptr;
    QAction *m_actionNewLayer = nullptr;
    QAction *m_actionShowRowTexts = nullptr;
    QAction *m_actionFilter = nullptr;
    QSlider *m_scaleSlider = nullptr;
    qt3dsdm::TSignalConnectionPtr m_connectSelectionChange;
    QIcon m_iconStop;
    QIcon m_iconPlay;
    QIcon m_iconDiActive;
    QIcon m_iconDiInactive;

    QString m_currController;

    DataInputSelectView *m_dataInputSelector;
};
#endif // TIMELINETOOLBAR_H
