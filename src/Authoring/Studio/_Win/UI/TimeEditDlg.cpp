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

//=============================================================================
// Prefix
//=============================================================================

#include "ui_timeeditdlg.h"

#include "stdafx.h"
#include "StudioPreferences.h"

//==============================================================================
//	 Includes
//==============================================================================

#include "TimeEditDlg.h"
#include "IDoc.h"
#include "Bindings/ITimelineKeyframesManager.h"

#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>

//=============================================================================
/**
 * Constructor
 */
CTimeEditDlg::CTimeEditDlg(QWidget *pParent)
    : QDialog(pParent)
    , m_MaxTime(0)
    , m_MaxTimeDisplay(0)
    , m_MinTimeDisplay(0)
    , m_InitialTime1(0)
    , m_InitialTime2(0)
    , m_MinOffset(0)
    , m_SecOffset(0)
    , m_MSecOffset(0)
    , m_MinMinorOffset(0)
    , m_SecMinorOffset(0)
    , m_MSecMinorOffset(0)
    , m_CursorPosition(0)
    , m_TimeFormat(0)
    , m_PreviousFormat(0)
    , m_TimeInMins(0.0f)
    , m_NumberOfDigitsDrop(0)
    , m_ColonPosition1(0)
    , m_ColonPosition2(0)
    , m_ObjectAssociation(0)
    , m_OffsetFromInitialTime(0)
    , m_Time1(0)
    , m_Time2(0)
    , m_Doc(nullptr)
    , m_KeyframesManager(nullptr)
    , m_Callback(nullptr)
    , m_ui(new Ui::TimeEditDlg)
{
    m_ui->setupUi(this);
    setAutoFillBackground(true);

    connect(m_ui->startTimeEdit, &QDateTimeEdit::timeChanged,
            this, &CTimeEditDlg::OnEnChangeTimeEdit1);
    connect(m_ui->endTimeEdit, &QDateTimeEdit::timeChanged,
            this, &CTimeEditDlg::OnEnChangeTimeEdit2);
}

//=============================================================================
/**
 * Destructor
 */
CTimeEditDlg::~CTimeEditDlg()
{
    delete m_ui;
}

void CTimeEditDlg::SetKeyframesManager(ITimelineKeyframesManager *inKeyframesManager)
{
    m_KeyframesManager = inKeyframesManager;
}

//=============================================================================
/**
 *  InitData: Initializes the Time Edit Dialog Box.
 *  @param inTime1 is the intial start time, which will be shown when the time edit
 *                 dialog box pops up
 *  @param inTime2 is the intial end time, which will be shown when the time edit
 *                 dialog box pops up
 *  @param inDoc this can be nullptr where its not applicable
 *	@param inObjectAssociation is the identifier for that identifies the object
 *							   associated with the time edit dialog
 *(e.g.
 *							   playhead, keyframe, timebar)
 */
void CTimeEditDlg::ShowDialog(long inTime1, long inTime2, IDoc *inDoc, long inObjectAssociation,
                              ITimeChangeCallback *inCallback /*= nullptr*/)
{
    m_InitialTime1 = inTime1;
    m_InitialTime2 = inTime2;
    m_ObjectAssociation = inObjectAssociation;
    m_Doc = inDoc;
    m_Callback = inCallback;

    m_OffsetFromInitialTime = 0;
    m_NumberOfDigitsDrop = 0;
    m_MinTimeDisplay = 0;
    // if it is a Timebar, this will be adjusted, else this should be initialized to some value at
    // least, for OverflowHandling to work correctly
    m_MaxTimeDisplay = LONG_MAX;

    // 9999:59:999 converted to milliseconds
    m_MaxTime =
            TimeConversion(9999, CONVERT_MIN_TO_MSEC) + TimeConversion(59,
                                                                       CONVERT_SEC_TO_MSEC) + 999;

    m_TimeDisplay2 = FormatTime(m_InitialTime2);
    // In cases Keyframes, where its only one set of time, do m_InitialTime1 later so that
    // m_Min, etc. values are initializd to the valid values.
    m_TimeDisplay1 = FormatTime(m_InitialTime1);

    // Present the dialog
    exec();
}

//==============================================================================
/**
 *	FormatTime: Called to break up a given time in milliseconds into min:sec:msec
 *				store it in a string. The string is formatted in 99:59:999 form.
 *	@param	inTime stores the time in milliseconds, which will be converted
 *			and stored in ioTimeString in 99:59:999 format.
 *  @return ioTimeString stores the result of the FormatTime method.
 */
QString CTimeEditDlg::FormatTime(long inTime)
{
    long theTime = inTime;
    long hour = 0;
    long min = 0;
    long sec = 0;
    long msec = 0;
    // Translates the m_InitialTime1 (in milliseconds) into Minutes, Seconds and Milliseconds
    if (inTime != 0) {
        min = TimeConversion(theTime, CONVERT_MSEC_TO_MIN);
        hour = min / 60;
        theTime = theTime - TimeConversion(min, CONVERT_MIN_TO_MSEC);
        min -= hour * 60;
        sec = TimeConversion(theTime, CONVERT_MSEC_TO_SEC);
        theTime = theTime - TimeConversion(sec, CONVERT_SEC_TO_MSEC);
        msec = theTime;
    }
    return QString::asprintf("%02d:%02d:%02d:%03d", hour, min, sec, msec);
}

//==============================================================================
/**
 *	OnInitDialog: Handle the WM_INITDIALOG message.
 *	@param	None
 *	@return Returns TRUE always.
 */
void CTimeEditDlg::showEvent(QShowEvent *ev)
{
    OnInitDialog();
    QDialog::showEvent(ev);
}


void CTimeEditDlg::OnInitDialog()
{
    // Hide the window items associated with end time.
    if (m_ObjectAssociation != TIMEBAR)
        HideUnnecessaryFields();
    m_ui->startTimeEdit->setTime(QTime::fromString(m_TimeDisplay1, "hh:mm:ss:zzz"));
    // Display the window captions for the correct object type
    switch (m_ObjectAssociation) {
    case PLAYHEAD:
        setWindowTitle(QObject::tr("Go To Time"));
        break;
    case ASSETKEYFRAME:
        setWindowTitle(QObject::tr("Set Keyframe Time"));
        break;
    case TIMEBAR:
        // m_TimeEditBoxNumber is a flag that switches the output between
        // the first and second text box.
        // When m_TimeEditBoxNumber = EDITBOX1, we are writing to the first text box
        // (the one that appears on top).
        // When it is EDITBOX2, the SetTimeEdit will write to the second text box.
        setWindowTitle(QObject::tr("Set Timebar Start/End Time"));
        m_ui->endTimeEdit->setTime(QTime::fromString(m_TimeDisplay2, "hh:mm:ss:zzz"));
        break;
    }
}
//==============================================================================
/**
 *  accept: Upon clicking ok, the dialog will be exited
 */
void CTimeEditDlg::accept()
{
    // Only commit here, cos dup keyframes will be deleted.
    if (m_ObjectAssociation == ASSETKEYFRAME && m_Doc && m_KeyframesManager)
        m_KeyframesManager->CommitChangedKeyframes();
    else if (m_ObjectAssociation == TIMEBAR)
        m_Callback->Commit();

    QDialog::accept();
}

//==============================================================================
/**
 *  reject: Upon clicking Cancel, revert to the initial time.
 */
void CTimeEditDlg::reject()
{
    // Only commit here, cos dup keyframes will be deleted.
    if (m_ObjectAssociation == ASSETKEYFRAME && m_Doc && m_KeyframesManager)
        m_KeyframesManager->RollbackChangedKeyframes();
    else if (m_ObjectAssociation == TIMEBAR)
        m_Callback->Rollback();
    QDialog::reject();
}

//==============================================================================
/**
 *	CountDigits: Counts the number of digits in a given number
 *  @param  inNumber is the number that is used to count the number of digits
 *  @return the number of digits
 */
long CTimeEditDlg::CountDigits(long inNumber)
{
    long theNumberOfDigits = 0;
    for (long theNumber = inNumber; theNumber >= 1; theNumber = theNumber / 10)
        theNumberOfDigits++;
    return theNumberOfDigits;
}


//==============================================================================
/**
 *	TimeOverflowUnderflow: Checks if inMin, inSec or inMSec has exceeded the overflow/
 *						   underflow limit. It will correct the time if it
 *overflow/
 *						   underflow.
 *	@param inMin stores the min segment of the time
 *	@param inSec stores the sec segment of the time
 *	@param inMSec stores the msec segment of the time
 *	@param inTimeLimit stores the time limit before the overflow/underflow occurs
 *	@param inOverflowOrUnderflow is set to true if we are checking for overflow else
 *								 we are checking for underflow
 *	@returns true if overflow/underflow occurs, otherwise returns false.
 */
bool CTimeEditDlg::TimeOverflowUnderflow(long *inMin, long *inSec, long *inMSec, long inTimeLimit,
                                         bool inOverflowOrUnderflow)
{
    // The codes below translates inTimeLimit into theLimitMin:theLimitSec:theLimitMsec
    bool theLimitExceeds = false;
    long theLimitMin;
    long theLimitSec;
    long theLimitMsec;
    TimeConversion(inTimeLimit, &theLimitMin, &theLimitSec, &theLimitMsec,
                   CONVERT_MSEC_TO_MIN_SEC_MSEC);
    // Handle time overflow/underflow
    if ((*inMin > theLimitMin && inOverflowOrUnderflow)
            || (*inMin < theLimitMin && inOverflowOrUnderflow == false)) {
        // Minutes exceeds limit
        // Set all time segments to the limit
        *inMin = theLimitMin;
        *inSec = theLimitSec;
        *inMSec = theLimitMsec;
        theLimitExceeds = true;
    } else if (*inMin == theLimitMin) {
        if ((*inSec > theLimitSec && inOverflowOrUnderflow)
                || (*inSec < theLimitSec && inOverflowOrUnderflow == false)) {
            // Seconds exceeds limit
            // Set the Sec and Msec segments to the limit
            *inSec = theLimitSec;
            *inMSec = theLimitMsec;
            theLimitExceeds = true;
        } else if (*inSec == theLimitSec) {
            if ((*inMSec > theLimitMsec && inOverflowOrUnderflow)
                    || (*inMSec < theLimitMsec
                        && inOverflowOrUnderflow == false)) {
                // Milliseconds exceeds limit
                // Msec segments to the limit
                *inMSec = theLimitMsec;
                theLimitExceeds = true;
            }
        }
    }
    return theLimitExceeds;
}

//==============================================================================
/**
 *	TimeConversion:			Converts inTime to the format specified by inFlags.
 *							For example:
 *							inTime = 5 sec inFlags = CONVERT_SEC_TO_MSEC
 *							The method will convert 5 sec into 5000 msec and
 *returns
 *							the result.
 *  @param  inTime			stores the time to be converted.
 *			inOperationCode determines the type of time conversion to be done on the
 *							inTime.
 *  @return theResult		stores the result of the time conversion.
 */
long CTimeEditDlg::TimeConversion(long inTime, long inOperationCode)
{
    long theResult = 0;
    switch (inOperationCode) {
    case CONVERT_MIN_TO_MSEC:
        theResult = inTime * 60 * 1000;
        break;
    case CONVERT_SEC_TO_MSEC:
        theResult = inTime * 1000;
        break;
    case CONVERT_MSEC_TO_MIN:
        theResult = inTime / (60 * 1000);
        break;
    case CONVERT_MSEC_TO_SEC:
        theResult = inTime / 1000;
        break;
    }
    return theResult;
}

//==============================================================================
/**
 *	TimeConversion:			Takes in the time in mins:secs:msec and convert it to
 *							the corresponding time in msec.
 *  @param  inMin			stores the minutes to be converted.
 *			inSec			stores the seconds to be converted.
 *			inMsec			stores the milliseconds to be converted.
 *			inOperationCode determines the type of time conversion to be done on the
 *							inMin, inSec and inMsec.
 *  @return theResult stores the result of the time conversion.
 */
long CTimeEditDlg::TimeConversion(long inMin, long inSec, long inMsec, long inOperationCode)
{
    long theResult = 0;
    switch (inOperationCode) {
    case CONVERT_TIME_TO_MSEC:
        theResult = TimeConversion(inMin, CONVERT_MIN_TO_MSEC)
                + TimeConversion(inSec, CONVERT_SEC_TO_MSEC) + inMsec;
        break;
    }
    return theResult;
}

//==============================================================================
/**
 *	TimeConversion:			Takes in the time in milliseconds and converts them
 *							to min : sec : msec.
 *  @param  inTotalTime		stores the total time in msec.
 *			ioMin			stores the mins result of the time conversion
 *			ioSec			stores the secs result of the time conversion
 *			ioMsec			stores the msecs result of the time conversion
 *			inOperationCode determines the type of time conversion to be done on the
 *							inTotalTime.
 *  @return NONE
 */
void CTimeEditDlg::TimeConversion(long inTotalTime, long *ioMin, long *ioSec, long *ioMsec,
                                  long inOperationCode)
{
    switch (inOperationCode) {
    case CONVERT_MSEC_TO_MIN_SEC_MSEC:
        *ioMin = TimeConversion(inTotalTime, CONVERT_MSEC_TO_MIN);
        *ioSec = inTotalTime - TimeConversion(*ioMin, CONVERT_MIN_TO_MSEC);
        *ioSec = TimeConversion(*ioSec, CONVERT_MSEC_TO_SEC);
        *ioMsec = inTotalTime - TimeConversion(*ioMin, CONVERT_MIN_TO_MSEC)
                - TimeConversion(*ioSec, CONVERT_SEC_TO_MSEC);
        break;
    }
}
//==============================================================================
/**
 *	UpdateObjectTime: It updates the playhead, keyframe or timebar time according
 *					  to the time displayed in the time edit dialogue.
 *  @param  inTime is the time that will be updated.
 *  @param  startTime if true, updates the start time on the object, otherwise the end time
 *  @return NONE
 */
void CTimeEditDlg::UpdateObjectTime(long inTime, bool startTime)
{
    long theDiff = 0;
    switch (m_ObjectAssociation) {
    case PLAYHEAD: // Update the playhead time
        if (m_Doc) {
            m_Doc->NotifyTimeChanged(inTime);
            m_Doc->NotifyTimeChanged(inTime);
        }
        break;
    case ASSETKEYFRAME: // Update the keyframe time
        if (m_Doc) {
            theDiff = inTime - m_OffsetFromInitialTime - m_InitialTime1;
            m_OffsetFromInitialTime = m_OffsetFromInitialTime + theDiff;
            if (theDiff != 0 && m_KeyframesManager) {
                m_KeyframesManager->OffsetSelectedKeyframes(theDiff);
            }
        }
        break;
    case TIMEBAR: // Update the timebar start/end time
        if (m_Callback) {
            if (startTime) // Update Start Time
                m_Callback->ChangeStartTime(inTime);
            else // Update End Time
                m_Callback->ChangeEndTime(inTime);
        }
        break;
    }
}

//==============================================================================
/**	The codes methods below are provides platform independent wrappers for
 *	User Interface.
 */
//==============================================================================

/**
 *	OnEnChangeTimeEdit:   Event triggered when the text in the 1st time edit box has
 *						  been changed.
 *  @param  NONE
 *  @return NONE
 */
void CTimeEditDlg::OnEnChangeTimeEdit1()
{
    // Making sure that the start time is not greater than the end time, when
    // the user modifies the start time of the timebar
    if (m_ObjectAssociation == TIMEBAR)
        m_MaxTimeDisplay = m_InitialTime2; // the initial end time
    m_MinTimeDisplay = 0;

    const auto time = m_ui->startTimeEdit->time();
    long min = time.minute() + time.hour() * 60;
    long sec = time.second();
    long msec = time.msec();
    TimeOverflowUnderflow(&min, &sec, &sec, m_MaxTimeDisplay, true);
    TimeOverflowUnderflow(&min, &sec, &msec, m_MinTimeDisplay, false);
    if (min != time.minute() || sec != time.second() ||
            msec != time.msec()) {
        m_ui->startTimeEdit->setTime(QTime(0, min, sec, msec));
    }
    m_ui->startTimeMin->setText(QString::number(min));
    m_ui->startTimeSec->setText(QString::number(sec));
    m_ui->startTimeMsec->setText(QString::number(msec));

    long theGoToTime = TimeConversion(min, CONVERT_MIN_TO_MSEC)
            + TimeConversion(sec, CONVERT_SEC_TO_MSEC) + msec;
    // Go to the time specified in the time edit display
    UpdateObjectTime(theGoToTime, true /*start time*/);
}

//==============================================================================
/**
 *	OnEnChangeTimeEdit2:   Event triggered when the text in the 2nd time edit box has
 *						   been changed.
 *  @param  NONE
 *  @return NONE
 */
void CTimeEditDlg::OnEnChangeTimeEdit2()
{
    // Let the end time of the time bar go as far as possible
    if (m_ObjectAssociation == TIMEBAR) {
        m_MaxTimeDisplay = m_MaxTime;
        m_MinTimeDisplay = m_InitialTime1; // the initial start time
    }
    const auto time = m_ui->endTimeEdit->time();
    long min = time.minute() + time.hour() * 60;
    long sec = time.second();
    long msec = time.msec();
    TimeOverflowUnderflow(&min, &sec, &sec, m_MaxTimeDisplay, true);
    TimeOverflowUnderflow(&min, &sec, &msec, m_MinTimeDisplay, false);
    if (min != time.minute() || sec != time.second() ||
            msec != time.msec()) {
        m_ui->endTimeEdit->setTime(QTime(0, min, sec, msec));
    }
    m_ui->endTimeMin->setText(QString::number(min));
    m_ui->endTimeSec->setText(QString::number(sec));
    m_ui->endTimeMsec->setText(QString::number(msec));

    long theGoToTime = TimeConversion(min, CONVERT_MIN_TO_MSEC)
            + TimeConversion(sec, CONVERT_SEC_TO_MSEC) + msec;
    // Go to the time specified in the time edit display
    UpdateObjectTime(theGoToTime, false /*end time*/);
}


//==============================================================================
/**
 *	HideUnnecessaryFields:   Hides unused dialog items
 *  @param  NONE
 *  @return NONE
 */
void CTimeEditDlg::HideUnnecessaryFields()
{
    m_ui->endTimeGroup->hide();
    m_ui->startTimeLabel->hide();
}

