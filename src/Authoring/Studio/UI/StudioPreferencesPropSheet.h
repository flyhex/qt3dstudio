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

#ifndef STUDIOPREFERENCESPROPSHEET_H
#define STUDIOPREFERENCESPROPSHEET_H

#pragma once

#include <QtWidgets/qdialog.h>

class CStudioProjectSettingsPage;
class CStudioPreferencesPropSheet;
class CStudioApp;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class StudioPreferencesPropSheet;
}
QT_END_NAMESPACE

class CStudioPreferencesPropPage : public QWidget
{
    Q_OBJECT
public:
    explicit CStudioPreferencesPropPage(QWidget *parent = nullptr);

    virtual bool onApply() { onOK(); setModified(false); return true; }
    virtual void onOK() {}
    virtual void onCancel() {}

protected:
    CStudioPreferencesPropSheet* sheet();

    void setModified(bool modified);
    void endDialog(int returnCode);
};

class CStudioPreferencesPropSheet : public QDialog
{
    Q_OBJECT
public:
    explicit CStudioPreferencesPropSheet(const QString &pszCaption, QWidget *pParentWnd = nullptr,
                                         int iSelectPage = 0);

protected:
    QFont m_Font; // Font for text

public:
    virtual ~CStudioPreferencesPropSheet();

protected:
    virtual void onInitDialog();

    bool apply();
    void accept() override;
    void reject() override;

private:
    QScopedPointer<QT_PREPEND_NAMESPACE(Ui::StudioPreferencesPropSheet)> m_ui;
};

#endif
