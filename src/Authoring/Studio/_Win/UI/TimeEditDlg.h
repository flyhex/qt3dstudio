/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

//==============================================================================
//	Prefix
//==============================================================================

#ifndef INCLUDED_TIME_EDIT_DIALOG_H
#define INCLUDED_TIME_EDIT_DIALOG_H 1

//==============================================================================
//	 Includes
//==============================================================================
#include "Dialogs.h"
#include <QDialog>
#include <QDateTimeEdit>

//==============================================================================
//	Forwards
//==============================================================================
class CTimebarControl;
class IDoc;
class ITimelineKeyframesManager;

//==============================================================================
//	Defines
//==============================================================================

enum ECursorPosition { MIN_POSITION, SEC_POSITION, MSEC_POSITION };

enum ETimeFormat { MSEC, SEC_MSEC, MIN_SEC_MSEC };

enum ETimeConversionOperation {
    CONVERT_MIN_TO_MSEC,
    CONVERT_SEC_TO_MSEC,
    CONVERT_MSEC_TO_MIN,
    CONVERT_MSEC_TO_SEC,
    CONVERT_TIME_TO_MSEC,
    CONVERT_MSEC_TO_MIN_SEC_MSEC
};

enum ETimeEditBoxNumber { EDITBOX1, EDITBOX2 };

enum EObjectAssociation {
    PLAYHEAD,
    ASSETKEYFRAME,
    TIMEBAR,
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
    class TimeEditDlg;
}
QT_END_NAMESPACE

// This class workaround the problem of QDateTimeEdit overwriting the selected section
// See https://bugreports.qt.io/browse/QTBUG-34759
class CDateTimeEdit : public QDateTimeEdit
{
  Q_OBJECT
public:
    using QDateTimeEdit::QDateTimeEdit;

protected:
    void focusInEvent(QFocusEvent *event) {
        QDateTimeEdit::focusInEvent(event);
        setSelectedSection(QDateTimeEdit::SecondSection);
    }
};

//==============================================================================
/**
 *	CTimeEditDlg: It is a dialog box that allows user to specify the time that
 *                he/she wishes to go to. This dialog box can be activated by
 *				  pressing Control G or clicking on the time edit box in the
 *				  timeline.
 */
//==============================================================================
class CTimeEditDlg : public QDialog
{
  Q_OBJECT

public:
    CTimeEditDlg(QWidget *pParent = nullptr); // standard constructor
    virtual ~CTimeEditDlg();
    void SetKeyframesManager(ITimelineKeyframesManager *inKeyframeManager);
    void ShowDialog(long inTime1, long inTime2, IDoc *inDoc, long inObjectAssociation,
                    ITimeChangeCallback *inCallback = nullptr);

public Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    void showEvent(QShowEvent *) override;

    // Generated message map functions
    void OnInitDialog();
    void OnEnChangeTimeEdit1();
    void OnEnChangeTimeEdit2();

    QString FormatTime(long inTime);
    long CountDigits(long inNumber);
    long TimeConversion(long inTime, long inOperationCode);
    long TimeConversion(long inMin, long inSec, long inMsec, long inOperationCode);
    void TimeConversion(long inTotalTime, long *ioMin, long *ioSec, long *ioMsec,
                        long inOperationCode);
    void UpdateObjectTime(long inTime, bool startTime);
    bool TimeOverflow(long *inMin, long *inSec, long *inMSec, long inTimeLimit);
    bool TimeOverflowUnderflow(long *inMin, long *inSec, long *inMSec, long inTimeLimit,
                               bool inOverflowOrUnderflow);
    void HideUnnecessaryFields();

protected:
    QString m_MinString;
    QString m_SecString;
    QString m_MSecString;
    QString m_TimeDisplay1;
    QString m_TimeDisplay2;
    long m_MaxTime;
    long m_MaxTimeDisplay;
    long m_MinTimeDisplay;
    long m_InitialTime1;
    long m_InitialTime2;
    long m_MinOffset;
    long m_SecOffset;
    long m_MSecOffset;
    long m_MinMinorOffset;
    long m_SecMinorOffset;
    long m_MSecMinorOffset;
    long m_CursorPosition;
    long m_TimeFormat;
    long m_PreviousFormat;
    double m_TimeInMins;
    long m_NumberOfDigitsDrop;
    long m_ColonPosition1;
    long m_ColonPosition2;
    long m_ObjectAssociation;
    long m_OffsetFromInitialTime;
    QString m_DTime;
    QString m_DTimeDisplay;
    long m_Time1;
    long m_Time2;
    IDoc *m_Doc;
    ITimelineKeyframesManager *m_KeyframesManager;

    ITimeChangeCallback *m_Callback;
    Ui::TimeEditDlg *m_ui;
};
#endif // INCLUDED_TIME_EDIT_DIALOG_H
