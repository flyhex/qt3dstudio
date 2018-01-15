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

#include "ui_DurationEditDlg.h"
#include "DurationEditDlg.h"
#include "IDoc.h"
#include "Bindings/ITimelineKeyframesManager.h"

//=============================================================================
/**
 * Constructor
 */
CDurationEditDlg::CDurationEditDlg(QWidget *pParent)
    : QDialog(pParent)
    , m_ui(new Ui::DurationEditDlg)
    , m_Doc(nullptr)
    , m_KeyframesManager(nullptr)
    , m_Callback(nullptr)
    , m_MaxTime(0)
    , m_MaxTimeDisplay(0)
    , m_MinTimeDisplay(0)
    , m_InitialTimeStart(0)
    , m_InitialTimeEnd(0)
    , m_minStart(-1)
    , m_secStart(-1)
    , m_minEnd(-1)
    , m_secEnd(-1)
{
    m_ui->setupUi(this);
    setAutoFillBackground(true);

    QIntValidator *minValidator = new QIntValidator(this);
    minValidator->setRange(0, 9999);
    m_ui->lineEditMinutes->setValidator(minValidator);
    m_ui->lineEditEndMinutes->setValidator(minValidator);
    QIntValidator *secValidator = new QIntValidator(this);
    secValidator->setRange(0, 59);
    m_ui->lineEditSeconds->setValidator(secValidator);
    m_ui->lineEditEndSeconds->setValidator(secValidator);
    QIntValidator *msecValidator = new QIntValidator(this);
    msecValidator->setRange(0, 999);
    m_ui->lineEditMilliseconds->setValidator(msecValidator);
    m_ui->lineEditEndMilliseconds->setValidator(msecValidator);

    connect(m_ui->lineEditMinutes, &QLineEdit::textEdited,
            this, &CDurationEditDlg::onStartTimeChanged);
    connect(m_ui->lineEditSeconds, &QLineEdit::textEdited,
            this, &CDurationEditDlg::onStartTimeChanged);
    connect(m_ui->lineEditMilliseconds, &QLineEdit::textEdited,
            this, &CDurationEditDlg::onStartTimeChanged);

    connect(m_ui->lineEditEndMinutes, &QLineEdit::textEdited,
            this, &CDurationEditDlg::onEndTimeChanged);
    connect(m_ui->lineEditEndSeconds, &QLineEdit::textEdited,
            this, &CDurationEditDlg::onEndTimeChanged);
    connect(m_ui->lineEditEndMilliseconds, &QLineEdit::textEdited,
            this, &CDurationEditDlg::onEndTimeChanged);
}

CDurationEditDlg::~CDurationEditDlg()
{
    delete m_ui;
}

void CDurationEditDlg::setKeyframesManager(ITimelineKeyframesManager *inKeyframesManager)
{
    m_KeyframesManager = inKeyframesManager;
}

//=============================================================================
/**
 *  showDialog: Initializes and shows the Duration Edit Dialog Box.
 *  @param startTime is the initial start time, which will be shown when the time edit
 *                   dialog box pops up
 *  @param endTime is the initial end time, which will be shown when the time edit
 *                 dialog box pops up
 *  @param inDoc this can be nullptr where its not applicable
 *  @param inCallback is the target object for the callbacks
 */
void CDurationEditDlg::showDialog(long startTime, long endTime, IDoc *inDoc,
                                  ITimeChangeCallback *inCallback)
{
    m_InitialTimeStart = startTime;
    m_InitialTimeEnd = endTime;
    m_Doc = inDoc;
    m_Callback = inCallback;

    m_MinTimeDisplay = 0;
    // if it is a Timebar, this will be adjusted, else this should be initialized to some value at
    // least, for OverflowHandling to work correctly
    m_MaxTimeDisplay = LONG_MAX;

    // 9999:59:999 converted to milliseconds
    m_MaxTime = timeConversion(9999, CONVERT_MIN_TO_MSEC)
            + timeConversion(59, CONVERT_SEC_TO_MSEC) + 999;

    // Set initial values to dialog
    formatTime(m_InitialTimeStart, true);
    formatTime(m_InitialTimeEnd, false);

    // Present the dialog
    exec();
}

void CDurationEditDlg::formatTime(long inTime, bool startTime)
{
    long theTime = inTime;
    long min = 0;
    long sec = 0;
    long msec = 0;

    // Translates the m_initialTime (in milliseconds) into Minutes, Seconds and Milliseconds
    if (inTime != 0) {
        min = timeConversion(theTime, CONVERT_MSEC_TO_MIN);
        theTime = theTime - timeConversion(min, CONVERT_MIN_TO_MSEC);
        sec = timeConversion(theTime, CONVERT_MSEC_TO_SEC);
        theTime = theTime - timeConversion(sec, CONVERT_SEC_TO_MSEC);
        msec = theTime;
    }

    if (startTime) {
        m_ui->lineEditMinutes->setText(QString::number(min));
        m_ui->lineEditSeconds->setText(QString::number(sec));
        m_ui->lineEditMilliseconds->setText(QString::number(msec));

        // Select the biggest non-zero unit
        if (min > 0) {
            m_ui->lineEditMinutes->setFocus();
            m_ui->lineEditMinutes->selectAll();
        } else if (sec > 0) {
            m_ui->lineEditSeconds->setFocus();
            m_ui->lineEditSeconds->selectAll();
        } else {
            m_ui->lineEditMilliseconds->setFocus();
            m_ui->lineEditMilliseconds->selectAll();
        }
    } else {
        m_ui->lineEditEndMinutes->setText(QString::number(min));
        m_ui->lineEditEndSeconds->setText(QString::number(sec));
        m_ui->lineEditEndMilliseconds->setText(QString::number(msec));
    }
}

void CDurationEditDlg::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);
}

void CDurationEditDlg::accept()
{
    m_Callback->Commit();
    QDialog::accept();
}

void CDurationEditDlg::reject()
{
    m_Callback->Rollback();
    QDialog::reject();
}

long CDurationEditDlg::numberOfDigits(long number)
{
    long theNumberOfDigits = 0;
    for (long theNumber = number; theNumber >= 1; theNumber = theNumber / 10)
        theNumberOfDigits++;
    return theNumberOfDigits;
}

//==============================================================================
/**
 *  timeConversion:         Converts inTime to the format specified by inFlags.
 *                          For example:
 *                          inTime = 5 sec inFlags = CONVERT_SEC_TO_MSEC
 *                          The method will convert 5 sec into 5000 msec and
 *                          returns the result.
 *  @param  inTime          stores the time to be converted.
 *          inOperationCode determines the type of time conversion to be done on the
 *                          inTime.
 *  @return theResult       stores the result of the time conversion.
 */
long CDurationEditDlg::timeConversion(long inTime, long inOperationCode)
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
 *  timeConversion:         Takes in the time in mins:secs:msec and convert it to
 *                          the corresponding time in msec.
 *  @param  inMin           stores the minutes to be converted.
 *          inSec           stores the seconds to be converted.
 *          inMsec          stores the milliseconds to be converted.
 *          inOperationCode determines the type of time conversion to be done on the
 *                          inMin, inSec and inMsec.
 *  @return theResult       stores the result of the time conversion.
 */
long CDurationEditDlg::timeConversion(long inMin, long inSec, long inMsec, long inOperationCode)
{
    long theResult = 0;
    switch (inOperationCode) {
    case CONVERT_TIME_TO_MSEC:
        theResult = timeConversion(inMin, CONVERT_MIN_TO_MSEC)
                + timeConversion(inSec, CONVERT_SEC_TO_MSEC) + inMsec;
        break;
    }
    return theResult;
}

//==============================================================================
/**
 *  timeConversion:         Takes in the time in milliseconds and converts them
 *                          to min : sec : msec.
 *  @param  inTotalTime     stores the total time in msec.
 *          ioMin           stores the mins result of the time conversion
 *          ioSec           stores the secs result of the time conversion
 *          ioMsec          stores the msecs result of the time conversion
 *          inOperationCode determines the type of time conversion to be done on the
 *                          inTotalTime.
 */
void CDurationEditDlg::timeConversion(long inTotalTime, long *ioMin, long *ioSec, long *ioMsec,
                                      long inOperationCode)
{
    switch (inOperationCode) {
    case CONVERT_MSEC_TO_MIN_SEC_MSEC:
        *ioMin = timeConversion(inTotalTime, CONVERT_MSEC_TO_MIN);
        *ioSec = inTotalTime - timeConversion(*ioMin, CONVERT_MIN_TO_MSEC);
        *ioSec = timeConversion(*ioSec, CONVERT_MSEC_TO_SEC);
        *ioMsec = inTotalTime - timeConversion(*ioMin, CONVERT_MIN_TO_MSEC)
                - timeConversion(*ioSec, CONVERT_SEC_TO_MSEC);
        break;
    }
}

void CDurationEditDlg::updateObjectTime(long inTime, bool startTime)
{
    if (m_Callback) {
        if (startTime)
            m_Callback->ChangeStartTime(inTime); // Update Start Time
        else
            m_Callback->ChangeEndTime(inTime); // Update End Time
    }
}

void CDurationEditDlg::onStartTimeChanged()
{
    // Making sure that the start time is not greater than the end time, when
    // the user modifies the start time of the timebar
    m_MaxTimeDisplay = m_InitialTimeEnd; // the initial end time
    m_MinTimeDisplay = 0;

    long min = m_ui->lineEditMinutes->text().toInt();
    long sec = m_ui->lineEditSeconds->text().toInt();
    long msec = m_ui->lineEditMilliseconds->text().toInt();

    long theGoToTime = timeConversion(min, CONVERT_MIN_TO_MSEC)
            + timeConversion(sec, CONVERT_SEC_TO_MSEC) + msec;

    // Go to the time specified in the start time edit display
    updateObjectTime(theGoToTime, true);

    // If max number of digits reached in a number field, select the next
    if (m_minStart != min && numberOfDigits(min) == 4) {
        m_ui->lineEditSeconds->setFocus();
        m_ui->lineEditSeconds->selectAll();
    } else if (m_secStart != sec && numberOfDigits(sec) == 2) {
        m_ui->lineEditMilliseconds->setFocus();
        m_ui->lineEditMilliseconds->selectAll();
    }

    m_minStart = min;
    m_secStart = sec;
}

void CDurationEditDlg::onEndTimeChanged()
{
    // Let the end time of the time bar go as far as possible
    m_MaxTimeDisplay = m_MaxTime;
    m_MinTimeDisplay = m_InitialTimeStart; // the initial start time

    long min = m_ui->lineEditEndMinutes->text().toInt();
    long sec = m_ui->lineEditEndSeconds->text().toInt();
    long msec = m_ui->lineEditEndMilliseconds->text().toInt();

    long theGoToTime = timeConversion(min, CONVERT_MIN_TO_MSEC)
            + timeConversion(sec, CONVERT_SEC_TO_MSEC) + msec;

    // Go to the time specified in the end time edit display
    updateObjectTime(theGoToTime, false);

    // If max number of digits reached in a number field, select the next
    if (m_minEnd != min && numberOfDigits(min) == 4) {
        m_ui->lineEditEndSeconds->setFocus();
        m_ui->lineEditEndSeconds->selectAll();
    } else if (m_secEnd != sec && numberOfDigits(sec) == 2) {
        m_ui->lineEditEndMilliseconds->setFocus();
        m_ui->lineEditEndMilliseconds->selectAll();
    }

    m_minEnd = min;
    m_secEnd = sec;
}
