/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

#ifndef TIME_EDIT_DIALOG_H
#define TIME_EDIT_DIALOG_H

#include "TimeEnums.h"
#include <QtWidgets/qdialog.h>

class CTimebarControl;
class IDoc;
class ITimelineKeyframesManager;

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class TimeEditDlg;
}
QT_END_NAMESPACE

class CTimeEditDlg : public QDialog
{
    Q_OBJECT

public:
    CTimeEditDlg(QWidget *pParent = nullptr); // standard constructor
    virtual ~CTimeEditDlg();
    void setKeyframesManager(ITimelineKeyframesManager *inKeyframeManager);
    void showDialog(long inTime, IDoc *inDoc, long inObjectAssociation);

public Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    void showEvent(QShowEvent *) override;

    void onInitDialog();
    void onTimeChanged();

    void formatTime(long inTime);
    int numberOfDigits(long number);
    long timeConversion(long inTime, long inOperationCode);
    long timeConversion(long inMin, long inSec, long inMsec, long inOperationCode);
    void timeConversion(long inTotalTime, long *ioMin, long *ioSec, long *ioMsec,
                        long inOperationCode);
    void updateObjectTime(long inTime);

protected:
    Ui::TimeEditDlg *m_ui;
    IDoc *m_Doc;
    ITimelineKeyframesManager *m_KeyframesManager;
    long m_InitialTime;
    long m_ObjectAssociation;
    long m_OffsetFromInitialTime;
    int m_min;
    int m_sec;
};
#endif // TIME_EDIT_DIALOG_H
