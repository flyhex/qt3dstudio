/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#ifndef INCLUDED_MASTER_CONTROL_H
#define INCLUDED_MASTER_CONTROL_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "ProceduralButton.h"

//==============================================================================
//	Forwards
//==============================================================================
class CStudioApp;
class CRenderer;
class CAssetControl;
class CGenericTabControl;
class CMasterControl;

//==============================================================================
/**
 *	@class	IMasterControlProvider
 */
class IMasterControlProvider
{
public:
    virtual void OnControlRemoved(CMasterControl *inControl) = 0;
    virtual void OnContextMenu(CMasterControl *inControl, const CPt &inPosition,
                               CContextMenu *inMyMenu) = 0;
    virtual void OnControlSelected(CMasterControl *inMaster, CControl *inControl, long inType) = 0;
};

//==============================================================================
/**
 *	@class	CMasterControl
 *	@brief	Class wrapping up all controls that appear in the Inspector Palette.
 */
class CMasterControl : public CControl
{
protected:
    typedef std::map<CControl *, long> TControlMap;

public:
    CMasterControl(IMasterControlProvider *inProvider);
    ~CMasterControl();

    void SetRenderDevice(UICRenderDevice inDevice) { m_RenderDevice = inDevice; }

    void AddControl(const Q3DStudio::CString &inName, long inType, CControl *inControl);
    void RemoveControl(CControl *inControl);
    void SelectControl(long inIndex);
    void SelectControl(CControl *inControl);
    long GetControlCount();
    CControl *FindControl(long inType);

    long GetActiveType();
    long GetActiveIndex();
    CControl *GetActiveControl();

    // CControl
    virtual Q3DStudio::CString GetName();
    void Draw(CRenderer *inRenderer) override;
    void SetSize(CPt inSize) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    UICRenderDevice GetPlatformDevice() override;
    void GrabFocus(CControl *inControl) override;

protected:
    void DisplayContextMenu(CPt inPosition);
    void OnTabSelected(CControl *inOldControl, CControl *inNewControl);
    void OnMenuButtonDown(CControl *);

protected:
    IMasterControlProvider *m_Provider; ///<
    CContextMenu *m_TabMenu; ///<
    Q3DStudio::CAutoMemPtr<CGenericTabControl> m_TabControl; ///<
    Q3DStudio::CAutoMemPtr<CProceduralButton<CButtonControl>>
        m_MenuButton; ///< menu button for the storage palette
    TControlMap m_ControlList; ///< the list of controls to display
    UICRenderDevice m_RenderDevice; ///<
};

#endif //#ifndef INCLUDED_MASTER_CONTROL_H
