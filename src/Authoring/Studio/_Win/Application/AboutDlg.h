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

#ifndef INCLUDED_ABOUT_DLG_H
#define INCLUDED_ABOUT_DLG_H 1

#pragma once

#include <QDialog>

#include "UICString.h"

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class AboutDlg;
}
QT_END_NAMESPACE

class CAboutDlg : public QDialog
{
    Q_OBJECT
public:
    CAboutDlg(QWidget* parent = nullptr);
    ~CAboutDlg();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void OnInitDialog();

private:
    QScopedPointer<Ui::AboutDlg> m_ui;

    QPalette *m_palette;

    Q3DStudio::CString m_ProductVersionStr;
    Q3DStudio::CString m_CopyrightStr;
    Q3DStudio::CString m_Credit1Str;
    Q3DStudio::CString m_Credit2Str;
};

#endif // INCLUDED_ABOUT_DLG_H
