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

#include <QtWidgets/qdialog.h>

class CTimebarControl;
class IDoc;
class IKeyframesManager;

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
    CTimeEditDlg(IKeyframesManager *keyframeManager);
    virtual ~CTimeEditDlg() override;
    void showDialog(long inTime, IDoc *inDoc, long inObjectAssociation);

public Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    void showEvent(QShowEvent *) override;

private:
    void onInitDialog();
    void onTimeChanged();

    void formatTime(long inTime);
    int numberOfDigits(long number);
    long timeConversion(long inTime, long inOperationCode);
    void updateObjectTime(long inTime);

    Ui::TimeEditDlg *m_ui = nullptr;
    IDoc *m_Doc = nullptr;
    IKeyframesManager *m_KeyframesManager = nullptr;
    long m_InitialTime = 0;
    long m_ObjectAssociation = 0;
    long m_OffsetFromInitialTime = 0;
    int m_min = -1;
    int m_sec = -1;
};
#endif // TIME_EDIT_DIALOG_H
