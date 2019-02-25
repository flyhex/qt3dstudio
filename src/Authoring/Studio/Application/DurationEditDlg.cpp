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
#include "TimeEnums.h"
#include <QtGui/qvalidator.h>

CDurationEditDlg::CDurationEditDlg(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::DurationEditDlg)
{
    m_ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false); // remove '?' from the dialog title bar

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

/**
 *  Initializes and shows the Duration Edit Dialog Box.
 *  @param startTime is the initial start time, which will be shown when the time edit
 *                   dialog box pops up
 *  @param endTime is the initial end time, which will be shown when the time edit
 *                 dialog box pops up
 *  @param inCallback is the target object for the callbacks
 */
void CDurationEditDlg::showDialog(long startTime, long endTime, ITimeChangeCallback *inCallback)
{
    m_Callback = inCallback;

    // Set initial values to dialog
    formatTime(startTime, true);
    formatTime(endTime, false);

    exec();
}

void CDurationEditDlg::formatTime(long inTime, bool startTime)
{
    long mins = 0;
    long secs = 0;
    long mils = 0;

    if (inTime != 0) {
        mins = inTime % 3600000 / 60000;
        secs = inTime % 60000 / 1000;
        mils = inTime % 1000;
    }

    // display milliseconds in 3 digits (5 -> 005)
    QString milsStr = QString("%1").arg(mils, 3, 10, QChar('0'));

    if (startTime) {
        m_ui->lineEditMinutes->setText(QString::number(mins));
        m_ui->lineEditSeconds->setText(QString::number(secs));
        m_ui->lineEditMilliseconds->setText(milsStr);

        // Select the biggest non-zero unit
        if (mins > 0) {
            m_ui->lineEditMinutes->setFocus();
            m_ui->lineEditMinutes->selectAll();
        } else if (secs > 0) {
            m_ui->lineEditSeconds->setFocus();
            m_ui->lineEditSeconds->selectAll();
        } else {
            m_ui->lineEditMilliseconds->setFocus();
            m_ui->lineEditMilliseconds->selectAll();
        }
    } else {
        m_ui->lineEditEndMinutes->setText(QString::number(mins));
        m_ui->lineEditEndSeconds->setText(QString::number(secs));
        m_ui->lineEditEndMilliseconds->setText(milsStr);
    }
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
    long min = m_ui->lineEditMinutes->text().toInt();
    long sec = m_ui->lineEditSeconds->text().toInt();
    long msec = m_ui->lineEditMilliseconds->text().toInt();

    long theGoToTime = min * 60000 + sec * 1000 + msec;

    // Go to the time specified in the start time edit display
    updateObjectTime(theGoToTime, true);

    // If max number of digits reached in a number field, select the next
    if (m_ui->lineEditMinutes->hasFocus() && min > 999) {
        m_ui->lineEditSeconds->setFocus();
        m_ui->lineEditSeconds->selectAll();
    } else if (m_ui->lineEditSeconds->hasFocus() && sec > 9) {
        m_ui->lineEditMilliseconds->setFocus();
        m_ui->lineEditMilliseconds->selectAll();
    }
}

void CDurationEditDlg::onEndTimeChanged()
{
    long min = m_ui->lineEditEndMinutes->text().toInt();
    long sec = m_ui->lineEditEndSeconds->text().toInt();
    long msec = m_ui->lineEditEndMilliseconds->text().toInt();

    long theGoToTime = min * 60000 + sec * 1000 + msec;

    // Go to the time specified in the end time edit display
    updateObjectTime(theGoToTime, false);

    // If max number of digits reached in a number field, select the next
    if (m_ui->lineEditEndMinutes->hasFocus() && min > 999) {
        m_ui->lineEditEndSeconds->setFocus();
        m_ui->lineEditEndSeconds->selectAll();
    } else if (m_ui->lineEditEndSeconds->hasFocus() && sec > 9) {
        m_ui->lineEditEndMilliseconds->setFocus();
        m_ui->lineEditEndMilliseconds->selectAll();
    }
}
