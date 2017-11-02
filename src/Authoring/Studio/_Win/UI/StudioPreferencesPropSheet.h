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

#if !defined(AFX_STUDIOPREFERENCESPROPSHEET_H__FADC69CE_5F0E_4F7E_A906_ED6052BBECF6__INCLUDED_)
#define AFX_STUDIOPREFERENCESPROPSHEET_H__FADC69CE_5F0E_4F7E_A906_ED6052BBECF6__INCLUDED_

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include <QDialog>

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

    virtual bool OnApply() { OnOK(); SetModified(false); return true; }
    virtual void OnOK() {}
    virtual void OnCancel() {}

protected:
    CStudioPreferencesPropSheet* sheet();

    void SetModified(bool modified);
    void EndDialog(int returnCode);
};

/////////////////////////////////////////////////////////////////////////////
// CStudioPreferencesPropSheet

class CStudioPreferencesPropSheet : public QDialog
{
    Q_OBJECT
    // Construction
public:
    explicit CStudioPreferencesPropSheet(int nIDCaption, QWidget *pParentWnd = nullptr,
                                         int iSelectPage = 0);
    explicit CStudioPreferencesPropSheet(const QString &pszCaption, QWidget *pParentWnd = nullptr,
                                         int iSelectPage = 0);

protected:
    QFont m_Font; // Font for text

    // Implementation
public:
    virtual ~CStudioPreferencesPropSheet();

    // Generated message map functions
protected:
    //{{AFX_MSG(CStudioPreferencesPropSheet)
    virtual void OnInitDialog();

    bool apply();
    void accept() override;
    void reject() override;

private:
    QScopedPointer<QT_PREPEND_NAMESPACE(Ui::StudioPreferencesPropSheet)> m_ui;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STUDIOPREFERENCESPROPSHEET_H__FADC69CE_5F0E_4F7E_A906_ED6052BBECF6__INCLUDED_)
