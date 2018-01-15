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

#ifndef DURATION_EDIT_DIALOG_H
#define DURATION_EDIT_DIALOG_H

#include <QtWidgets/qdialog.h>

class CTimebarControl;
class IDoc;
class ITimelineKeyframesManager;

enum ETimeFormat { MSEC, SEC_MSEC, MIN_SEC_MSEC };

enum ETimeConversionOperation {
    CONVERT_MIN_TO_MSEC,
    CONVERT_SEC_TO_MSEC,
    CONVERT_MSEC_TO_MIN,
    CONVERT_MSEC_TO_SEC,
    CONVERT_TIME_TO_MSEC,
    CONVERT_MSEC_TO_MIN_SEC_MSEC
};

class ITimeChangeCallback
{
public:
    virtual ~ITimeChangeCallback() {}
    virtual void ChangeStartTime(long) = 0;
    virtual void ChangeEndTime(long) = 0;
    virtual void Commit() = 0;
    virtual void Rollback() = 0;
};

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class DurationEditDlg;
}
QT_END_NAMESPACE

class CDurationEditDlg : public QDialog
{
    Q_OBJECT

public:
    CDurationEditDlg(QWidget *pParent = nullptr); // standard constructor
    virtual ~CDurationEditDlg();
    void setKeyframesManager(ITimelineKeyframesManager *inKeyframeManager);
    void showDialog(long startTime, long endTime, IDoc *inDoc,
                    ITimeChangeCallback *inCallback = nullptr);

public Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    void showEvent(QShowEvent *) override;

    void onStartTimeChanged();
    void onEndTimeChanged();

    void formatTime(long inTime, bool startTime);
    long numberOfDigits(long number);
    long timeConversion(long inTime, long inOperationCode);
    long timeConversion(long inMin, long inSec, long inMsec, long inOperationCode);
    void timeConversion(long inTotalTime, long *ioMin, long *ioSec, long *ioMsec,
                        long inOperationCode);
    void updateObjectTime(long inTime, bool startTime);

protected:
    Ui::DurationEditDlg *m_ui;
    IDoc *m_Doc;
    ITimelineKeyframesManager *m_KeyframesManager;
    ITimeChangeCallback *m_Callback;
    long m_MaxTime;
    long m_MaxTimeDisplay;
    long m_MinTimeDisplay;
    long m_InitialTimeStart;
    long m_InitialTimeEnd;
    int m_minStart;
    int m_secStart;
    int m_minEnd;
    int m_secEnd;
};
#endif // DURATION_EDIT_DIALOG_H
