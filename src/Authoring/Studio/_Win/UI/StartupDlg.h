/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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
#ifndef INCLUDED_STARTUP_DLG
#define INCLUDED_STARTUP_DLG 1

#pragma once

//==============================================================================
//	 Includes
//==============================================================================

#include <QDialog>

#include "Qt3DSString.h"
#include "Qt3DSFile.h"

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class StartupDlg;
}
QT_END_NAMESPACE

//==============================================================================
/**
 *	CGLVersionDlg: Dialog class for showing what user can do upon startup
 */
//==============================================================================
class CStartupDlg : public QDialog
{
    Q_OBJECT
public:
    enum EStartupChoice {
        EStartupChoice_Invalid = -1,
        EStartupChoice_NewDoc,
        EStartupChoice_OpenDoc,
        EStartupChoice_OpenRecent,
        EStartupChoice_Exit
    };

public:
    CStartupDlg(QWidget *pParent = nullptr); // standard constructor
    virtual ~CStartupDlg();

protected:
    const static int RECENT_COUNT = 5;

    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *) override;
    void reject() override;

protected Q_SLOTS:
    void OnNewDocClicked();
    void OnOpenDocClicked();
    void OnStnClickedStartupRecent();
    void OpenRecent(size_t inIndex);

private:
    // Dialog background
    QPalette *m_palette;

    // Product version string
    Q3DStudio::CString m_ProductVersionStr;

    // Choice
    EStartupChoice m_Choice = EStartupChoice_Invalid;

    // Recent Docs
    std::vector<CUICFile> m_RecentDocs;
    CUICFile m_RecentDocSelected;

public:
    void OnInitDialog();
    void AddRecentItem(const CUICFile &inRecentItem);
    EStartupChoice GetChoice();
    CUICFile GetRecentDoc() const;

private:
    QScopedPointer<Ui::StartupDlg> m_ui;
};

#endif // INCLUDED_STARTUP_DLG
