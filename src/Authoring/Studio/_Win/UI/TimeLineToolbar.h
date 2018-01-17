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

#ifndef INCLUDED_TIMELINETOOLBAR_H
#define INCLUDED_TIMELINETOOLBAR_H 1

#include "Qt3DSDMSignals.h"
#include "SelectedValueImpl.h"
#include "DataInputSelectDlg.h"
#include "DispatchListeners.h"
#include "Dispatch.h"
#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class TimeLineToolbar;
}
QT_END_NAMESPACE

class CMainFrame;

class TimeLineToolbar : public QWidget,
                        public IDataModelListener
{
  Q_OBJECT
public:
    TimeLineToolbar(CMainFrame *mainFrame, const QSize &preferredSize, QWidget *pParent = nullptr);
    virtual ~TimeLineToolbar();

    void onTimeChanged(long time);
    void OnSelectionChange(Q3DStudio::SSelectedValue newSelectable);

    void showDataInputChooser();
    void onDataInputChange(const QString &dataInputName);

    QSize sizeHint() const;

    // IDataModelListener
    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                            long inInstanceCount) override;
private Q_SLOTS:
    void onAddLayerClicked();
    void onPlayButtonClicked();
    void onAddDataInputClicked();

protected:
    QT_PREPEND_NAMESPACE(Ui::TimeLineToolbar) *m_ui;
    QSize m_preferredSize;
    CMainFrame *m_mainFrame;
    qt3dsdm::Qt3DSDMInstanceHandle m_currTimeCtxRoot = 0;
    QString m_currController;

    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_Connections;
    DataInputSelectDlg *m_DataInputSelector;
    void updateDataInputStatus(bool isViaDispatch);
};
#endif // INCLUDED_TIMELINETOOLBAR_H
