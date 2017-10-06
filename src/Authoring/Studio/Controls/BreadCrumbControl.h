/****************************************************************************
**
** Copyright (C) 2004 NVIDIA Corporation.
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

#ifndef INCLUDED_BREADCRUMBCONTROL_H
#define INCLUDED_BREADCRUMBCONTROL_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "FlowLayout.h"
#include "ProceduralButton.h"
#include "ToggleButton.h"
#include "TextButton.h"
#include "IBreadCrumbProvider.h"

//==============================================================================
//	Forwards
//==============================================================================
class IDoc;
class CRenderer;

class CBreadCrumbControl : public CFlowLayout
{
public:
    typedef CProceduralButton<CTextButton<CToggleButton>> TButtonType;
    typedef CTextButton<CButtonControl> TSeparatorButtonType;
    struct SBreadCrumbItem
    {
        TButtonType *m_BreadCrumb;
        TSeparatorButtonType *m_Separator;

        SBreadCrumbItem(TButtonType *inBreadCrumb, TSeparatorButtonType *inSeparator)
        {
            m_BreadCrumb = inBreadCrumb;
            m_Separator = inSeparator;
        }
    };
    typedef std::vector<SBreadCrumbItem> TBreadCrumbList;

    // Construction
public:
    CBreadCrumbControl();
    virtual ~CBreadCrumbControl();

    DEFINE_OBJECT_COUNTER(CBreadCrumbControl)

    // CControl
public:
    virtual void Draw(CRenderer *inRenderer);

    void RefreshTrail(IBreadCrumbProvider *inBreadCrumbProvider);
    void OnUpdateBreadCrumb();

    // Implementation
protected:
    void GenerateButtonText(const SBreadCrumb &inBreadCrumb, TButtonType *inButton);
    void OnButtonToggled(CToggleButton *inButton, CButtonControl::EButtonState inState);
    void RemoveButton(SBreadCrumbItem &inBreadCrumb);
    void RemoveAllButtons();

protected:
    IBreadCrumbProvider *m_BreadCrumbProvider;
    TBreadCrumbList m_BreadCrumbList;
};

#endif // INCLUDED_BREADCRUMBCONTROL_H
