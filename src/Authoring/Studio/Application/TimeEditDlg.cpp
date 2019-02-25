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
#include "KeyframeManager.h"
#include "IDoc.h"
#include "TimeEnums.h"
#include <QtGui/qvalidator.h>

CTimeEditDlg::CTimeEditDlg(KeyframeManager *keyframeManager)
    : m_ui(new Ui::TimeEditDlg)
    , m_keyframeManager(keyframeManager)
{
    m_ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false); // remove '?' from the dialog title bar

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

/**
 *  Initializes and shows the dialog
 *  @param inTime the initial time which will be shown when the dialog pops up
 *  @param inDoc this can be nullptr where its not applicable
 *  @param inObjectAssociation the identifier for the object associated with the dialog (playhead
 *                             or keyframe)
 */
void CTimeEditDlg::showDialog(long inTime, IDoc *inDoc, long inObjectAssociation)
{
    m_initialTime = inTime;
    m_objectAssociation = inObjectAssociation;
    m_Doc = inDoc;

    // Set initial values to dialog
    formatTime(m_initialTime);

    exec();
}

void CTimeEditDlg::formatTime(long inTime)
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
}

void CTimeEditDlg::showEvent(QShowEvent *e)
{
    onInitDialog();
    QDialog::showEvent(e);
}

void CTimeEditDlg::onInitDialog()
{
    QString title;
    // Display the window captions for the correct object type
    switch (m_objectAssociation) {
    case PLAYHEAD:
        title = QObject::tr("Go To Time");
        break;
    case ASSETKEYFRAME:
        title = QObject::tr("Set Keyframe Time");
        Q_ASSERT(m_keyframeManager != nullptr);
        break;
    }
    setWindowTitle(title);
    m_ui->labelTitle->setText(title);
}

void CTimeEditDlg::accept()
{
    // Only commit here, cos dup keyframes will be deleted.
    if (m_objectAssociation == ASSETKEYFRAME && m_Doc) {
        if (m_endTime == m_initialTime)
            m_keyframeManager->RollbackChangedKeyframes();
        else
            m_keyframeManager->CommitChangedKeyframes();
    }

    QDialog::accept();
}

void CTimeEditDlg::reject()
{
    // Only commit here, cos dup keyframes will be deleted.
    if (m_objectAssociation == ASSETKEYFRAME && m_Doc)
        m_keyframeManager->RollbackChangedKeyframes();
    QDialog::reject();
}

/**
 *  Updates the playhead or keyframe time according to the time displayed in the time edit dialogue.
 *  @param  inTime  the time that will be updated.
 */
void CTimeEditDlg::updateObjectTime(long inTime)
{
    switch (m_objectAssociation) {
    case PLAYHEAD: // Update the playhead time
        if (m_Doc)
            m_Doc->NotifyTimeChanged(inTime);
        break;
    case ASSETKEYFRAME: // Update the keyframe time
        if (m_Doc) {
            m_endTime = inTime;
            m_keyframeManager->moveSelectedKeyframes(inTime);
        }
        break;
    }
}

void CTimeEditDlg::onTimeChanged()
{
    long min = m_ui->lineEditMinutes->text().toInt();
    long sec = m_ui->lineEditSeconds->text().toInt();
    long msec = m_ui->lineEditMilliseconds->text().toInt();

    long theGoToTime = min * 60000 + sec * 1000 + msec;

    // make sure min keyframe time doesn't go below zero
    long offset = m_keyframeManager->getPressedKeyframeOffset();
    if (theGoToTime - offset < 0) {
        theGoToTime = offset;
        formatTime(theGoToTime);
    }
    // Go to the time specified in the time edit display
    updateObjectTime(theGoToTime);

    // If max number of digits reached in a number field, select the next
    if (m_ui->lineEditMinutes->hasFocus() && min > 999) {
        m_ui->lineEditSeconds->setFocus();
        m_ui->lineEditSeconds->selectAll();
    } else if (m_ui->lineEditSeconds->hasFocus() && sec > 9) {
        m_ui->lineEditMilliseconds->setFocus();
        m_ui->lineEditMilliseconds->selectAll();
    }
}
