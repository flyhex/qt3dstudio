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

public:
    TimelineToolbar();
    virtual ~TimelineToolbar();
    void setTime(long totalMillis);
    QString getCurrentController() const;
    void setNewLayerEnabled(bool enable);
    QAction *actionShowRowTexts() const;

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
    void onShowRowTextsToggled();

private:
    void addSpacing(int width);
    void onSelectionChange(Q3DStudio::SSelectedValue inNewSelectable);
    void onDataInputChange(int handle, int instance, const QString &dataInputName);
    void showDataInputChooser(const QPoint &point);
    void updateDataInputStatus();
    void updateTimelineTitleColor(bool controlled);

    QPushButton *m_timeLabel;
    QLabel *m_diLabel;
    QAction *m_actionDeleteRow;
    QAction *m_actionPlayStop;
    QAction *m_actionZoomIn;
    QAction *m_actionZoomOut;
    QAction *m_actionDataInput;
    QAction *m_actionNewLayer;
    QAction *m_actionShowRowTexts;
    qt3dsdm::TSignalConnectionPtr m_connectSelectionChange;
    QSlider *m_scaleSlider;
    QIcon m_iconStop;
    QIcon m_iconPlay;
    QIcon m_iconDiActive;
    QIcon m_iconDiInactive;
    QIcon m_iconTimebarTextsActive;
    QIcon m_iconTimebarTextsInactive;

    QString m_currController;

    DataInputSelectView *m_dataInputSelector;
};
#endif // TIMELINETOOLBAR_H
