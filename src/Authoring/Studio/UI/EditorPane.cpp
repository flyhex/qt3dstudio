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

#include "Qt3DSCommonPrecompile.h"
#include "EditorPane.h"

//==============================================================================
/**
 *	Constructor:	Creates a CEditorPane
 */
CEditorPane::CEditorPane()
    : m_EditObject(nullptr)
{
}

//==============================================================================
/**
 *	Destructor:	Releases the CEditorPane
 */
CEditorPane::~CEditorPane()
{
}

//==============================================================================
/**
 *	Set the object that this window will edit.
 *	Recieve and keep the CAsset to be edited.  You can override this method
 *	for each specific editor view class.
 *	@param inEditObject The CAsset to be represented and edited in this view.
 */
void CEditorPane::SetEditObject(CAsset *inEditObject)
{
    // Retain this object
    m_EditObject = inEditObject;
}

//==============================================================================
/**
 *	Get the object that this window is editing.
 *	return The CAsset that is associated with this window
 */
CAsset *CEditorPane::GetEditObject()
{
    // Return the asset
    return m_EditObject;
}

//==============================================================================
/**
 *	CloseEditor: Close the editor.
 *	Override to close each individual editor.
 *	@return false
 */
bool CEditorPane::CloseEditor()
{
    return false;
}
