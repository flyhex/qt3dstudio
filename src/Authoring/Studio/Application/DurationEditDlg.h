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

class IDoc;

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
    CDurationEditDlg(QWidget *parent = nullptr);
    ~CDurationEditDlg() override;

    void showDialog(long startTime, long endTime, ITimeChangeCallback *inCallback = nullptr);

public Q_SLOTS:
    void accept() override;
    void reject() override;

private:
    void onStartTimeChanged();
    void onEndTimeChanged();

    void formatTime(long inTime, bool startTime);
    void updateObjectTime(long inTime, bool startTime);

    Ui::DurationEditDlg *m_ui;
    ITimeChangeCallback *m_Callback = nullptr;
};
#endif // DURATION_EDIT_DIALOG_H
