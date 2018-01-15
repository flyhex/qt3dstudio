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

#include "ui_TimeEditDlg.h"
#include "TimeEditDlg.h"
#include "IDoc.h"
#include "Bindings/ITimelineKeyframesManager.h"

CTimeEditDlg::CTimeEditDlg(QWidget *pParent)
    : QDialog(pParent)
    , m_ui(new Ui::TimeEditDlg)
    , m_Doc(nullptr)
    , m_KeyframesManager(nullptr)
    , m_InitialTime(0)
    , m_ObjectAssociation(0)
    , m_OffsetFromInitialTime(0)
    , m_min(-1)
    , m_sec(-1)
{
    m_ui->setupUi(this);
    setAutoFillBackground(true);

    QIntValidator *minValidator = new QIntValidator(this);
    minValidator->setRange(0, 9999);
    m_ui->lineEditMinutes->setValidator(minValidator);
    QIntValidator *secValidator = new QIntValidator(this);
    secValidator->setRange(0, 59);
    m_ui->lineEditSeconds->setValidator(secValidator);
    QIntValidator *msecValidator = new QIntValidator(this);
    msecValidator->setRange(0, 999);
    m_ui->lineEditMilliseconds->setValidator(msecValidator);

    connect(m_ui->lineEditMinutes, &QLineEdit::textEdited, this, &CTimeEditDlg::onTimeChanged);
    connect(m_ui->lineEditSeconds, &QLineEdit::textEdited, this, &CTimeEditDlg::onTimeChanged);
    connect(m_ui->lineEditMilliseconds, &QLineEdit::textEdited, this, &CTimeEditDlg::onTimeChanged);
}

CTimeEditDlg::~CTimeEditDlg()
{
    delete m_ui;
}

void CTimeEditDlg::setKeyframesManager(ITimelineKeyframesManager *inKeyframesManager)
{
    m_KeyframesManager = inKeyframesManager;
}

//=============================================================================
/**
 *  showDialog: Initializes and shows the Time Edit Dialog Box.
 *  @param inTime is the initial time, which will be shown when the time edit
 *                dialog box pops up
 *  @param inDoc this can be nullptr where its not applicable
 *  @param inObjectAssociation is the identifier for that identifies the object
 *                             associated with the time edit dialog
 *                             (e.g. playhead, keyframe)
 */
void CTimeEditDlg::showDialog(long inTime, IDoc *inDoc, long inObjectAssociation)
{
    m_InitialTime = inTime;
    m_ObjectAssociation = inObjectAssociation;
    m_Doc = inDoc;

    // Set initial values to dialog
    formatTime(m_InitialTime);

    exec();
}

void CTimeEditDlg::formatTime(long inTime)
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
}

void CTimeEditDlg::showEvent(QShowEvent *ev)
{
    onInitDialog();
    QDialog::showEvent(ev);
}

void CTimeEditDlg::onInitDialog()
{
    QString title;
    // Display the window captions for the correct object type
    switch (m_ObjectAssociation) {
    case PLAYHEAD:
        title = QObject::tr("Go To Time");
        break;
    case ASSETKEYFRAME:
        title = QObject::tr("Set Keyframe Time");
        break;
    }
    setWindowTitle(title);
    m_ui->labelTitle->setText(title);
}

void CTimeEditDlg::accept()
{
    // Only commit here, cos dup keyframes will be deleted.
    if (m_ObjectAssociation == ASSETKEYFRAME && m_Doc && m_KeyframesManager)
        m_KeyframesManager->CommitChangedKeyframes();

    QDialog::accept();
}

void CTimeEditDlg::reject()
{
    // Only commit here, cos dup keyframes will be deleted.
    if (m_ObjectAssociation == ASSETKEYFRAME && m_Doc && m_KeyframesManager)
        m_KeyframesManager->RollbackChangedKeyframes();
    QDialog::reject();
}

int CTimeEditDlg::numberOfDigits(long number)
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
long CTimeEditDlg::timeConversion(long inTime, long inOperationCode)
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
long CTimeEditDlg::timeConversion(long inMin, long inSec, long inMsec, long inOperationCode)
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
void CTimeEditDlg::timeConversion(long inTotalTime, long *ioMin, long *ioSec, long *ioMsec,
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

//==============================================================================
/**
 *  updateObjectTime: It updates the playhead or keyframe time according
 *                    to the time displayed in the time edit dialogue.
 *  @param  inTime is the time that will be updated.
 */
void CTimeEditDlg::updateObjectTime(long inTime)
{
    long theDiff = 0;
    switch (m_ObjectAssociation) {
    case PLAYHEAD: // Update the playhead time
        if (m_Doc)
            m_Doc->NotifyTimeChanged(inTime);
        break;
    case ASSETKEYFRAME: // Update the keyframe time
        if (m_Doc) {
            theDiff = inTime - m_OffsetFromInitialTime - m_InitialTime;
            m_OffsetFromInitialTime = m_OffsetFromInitialTime + theDiff;
            if (theDiff != 0 && m_KeyframesManager)
                m_KeyframesManager->OffsetSelectedKeyframes(theDiff);
        }
        break;
    }
}

void CTimeEditDlg::onTimeChanged()
{
    long min = m_ui->lineEditMinutes->text().toInt();
    long sec = m_ui->lineEditSeconds->text().toInt();
    long msec = m_ui->lineEditMilliseconds->text().toInt();

    long theGoToTime = timeConversion(min, CONVERT_MIN_TO_MSEC)
            + timeConversion(sec, CONVERT_SEC_TO_MSEC) + msec;

    // Go to the time specified in the time edit display
    updateObjectTime(theGoToTime);

    // If max number of digits reached in a number field, select the next
    if (m_min != min && numberOfDigits(min) == 4) {
        m_ui->lineEditSeconds->setFocus();
        m_ui->lineEditSeconds->selectAll();
    } else if (m_sec != sec && numberOfDigits(sec) == 2) {
        m_ui->lineEditMilliseconds->setFocus();
        m_ui->lineEditMilliseconds->selectAll();
    }

    m_min = min;
    m_sec = sec;
}
