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

//==============================================================================
//	Prefix
//==============================================================================

#ifndef INCLUDED_TEXTEDITCONTEXTMENU_H
#define INCLUDED_TEXTEDITCONTEXTMENU_H 1

//==============================================================================
//	Includes
//==============================================================================
#include <QtWidgets/qmenu.h>

//==============================================================================
//	Forwards
//==============================================================================
class CTextEdit;

//==============================================================================
/**
 * @class CTextEditContextMenu
 */

class CTextEditContextMenu : public QMenu
{
    Q_OBJECT
public:
    CTextEditContextMenu(CTextEdit *inEditControl, QWidget *parent = nullptr);
    ~CTextEditContextMenu();

protected Q_SLOTS:
    void CutText();
    void CopyText();
    void PasteText();

protected:
    void showEvent(QShowEvent *event) override;

    bool CanCutText();
    bool CanPasteText();
    bool CanCopyText();

    CTextEdit *m_TextEditControl;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
};

#endif // INCLUDED_TEXTEDITCONTEXTMENU_H
