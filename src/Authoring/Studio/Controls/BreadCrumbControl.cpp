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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "BreadCrumbControl.h"
#include "StudioPreferences.h"
#include "IBreadCrumbProvider.h"

#include <QtGui/qpixmap.h>

IMPLEMENT_OBJECT_COUNTER(CBreadCrumbControl)

//==============================================================================
// Internal Classes
//==============================================================================

//=============================================================================
/**
 *	Constructor
 */
CBreadCrumbControl::CBreadCrumbControl()
    : m_BreadCrumbProvider(nullptr)
{
    ADDTO_OBJECT_COUNTER(CBreadCrumbControl)

    SetName("BreadCrumbControl");
    SetAutoMin(false);
    SetFlowDirection(FLOW_HORIZONTAL);
    SetAlignment(ALIGN_TOP, ALIGN_LEFT);
    SetTopMargin(0);
    SetBottomMargin(0);
    SetLeftMargin(0);
    SetRightMargin(0);
    SetGapBetweenChildren(0);

    long theHeaderHeight = CStudioPreferences::GetHeaderHeight();
    SetMaximumSize(CPt(LONG_MAX, theHeaderHeight));
    SetMinimumSize(CPt(0, theHeaderHeight));
    SetSize(CPt(LONG_MAX, theHeaderHeight));
    SetPosition(CPt(0, 0));
}

//=============================================================================
/**
 * destructor
 */
CBreadCrumbControl::~CBreadCrumbControl()
{
    RemoveAllButtons();
    REMOVEFROM_OBJECT_COUNTER(CBreadCrumbControl)
}

//=============================================================================
/**
 * Helper class to create the colon separator button
 */
static inline CBreadCrumbControl::TSeparatorButtonType *CreateSeparatorButton(const QPixmap &inImage)
{
    CBreadCrumbControl::TSeparatorButtonType *theButton =
            new CBreadCrumbControl::TSeparatorButtonType;
    theButton->SetAutoSize(true);
    theButton->SetUpImage(inImage);
    theButton->SetSize(CPt(12, 24));
    theButton->SetMinimumSize(CPt(12, 24));
    theButton->SetMaximumSize(CPt(12, 24));
    theButton->SetCenterImage(true, false);
    return theButton;
}

//=============================================================================
/**
 * Helper function to create a new button to be placed on the breadcrumb bar
 * @param inButtonName Name of the button to be displayed and stored
 *        inImageName  The specific .png file to load in for the button
 * @return TButtonType returns the newly created button
 */
static inline CBreadCrumbControl::TButtonType *CreateButton(const QPixmap &inImage)
{
    CBreadCrumbControl::TButtonType *theButton = new CBreadCrumbControl::TButtonType();
    theButton->SetAutoSize(true);
    theButton->SetUpImage(inImage);
    CBreadCrumbControl::TButtonType::SBorderOptions theBorderOptions(false, false, false, false);
    theButton->SetBorderVisibilityAll(theBorderOptions);
    theButton->SetFillStyleUp(CBreadCrumbControl::TButtonType::EFILLSTYLE_FLOOD);
    theButton->SetFillStyleOver(CBreadCrumbControl::TButtonType::EFILLSTYLE_FLOOD);
    theButton->SetFillColorUp(CStudioPreferences::GetBaseColor());
    theButton->SetFillColorDown(CStudioPreferences::GetBaseColor());
    theButton->SetFillColorOver(CStudioPreferences::GetMouseOverHighlightColor());
    theButton->SetMinimumSize(CPt(16, 24));
    theButton->SetMaximumSize(CPt(125, 24));
    theButton->SetCenterImage(true, false);
    return theButton;
}

//=============================================================================
/**
 *	Generate/Reset the text appearing on the breadcrumb which is currently
 *	appending the slide name as well.
 *	@param	inBreadCrumb	containing color/text info for display
 *	@param	inButton	The button to be modified
 */
void CBreadCrumbControl::GenerateButtonText(const SBreadCrumb &inBreadCrumb, TButtonType *inButton)
{
    QString theDisplayName(inBreadCrumb.m_String);
    inButton->SetTextColorUp(CStudioPreferences::GetNormalColor());
    inButton->SetTextColorDown(CStudioPreferences::GetNormalColor());
    inButton->SetText(theDisplayName);
    inButton->SetName(theDisplayName);
}

//=============================================================================
/**
 * Draws this control
 */
void CBreadCrumbControl::Draw(CRenderer *inRenderer)
{
    const auto size = GetSize();

    // Fill in the background
    inRenderer->FillSolidRect(QRect(0, 0, size.x, size.y), CStudioPreferences::GetBaseColor());

    // Outline the control
    CColor theOutlineColor = CStudioPreferences::GetButtonShadowColor();
    inRenderer->DrawRectOutline(QRect(0, 0, size.x, size.y -1), theOutlineColor, theOutlineColor,
                                theOutlineColor, theOutlineColor);

    // Draw the highlight at the bottom
    inRenderer->PushPen(CStudioPreferences::GetButtonHighlightColor());
    inRenderer->MoveTo(QPoint(0, size.y - 1));
    inRenderer->LineTo(QPoint(size.x - 1, size.y - 1));
    inRenderer->PopPen();
}

//=============================================================================
/**
 *	Update the entire bread crumb trail. This will 'reuse' created controls, creating more if
 *trail is longer than previous, and deleting extra if shorter.
 */
void CBreadCrumbControl::RefreshTrail(IBreadCrumbProvider *inBreadCrumbProvider)
{
    bool theChangedProvider = m_BreadCrumbProvider != inBreadCrumbProvider;
    if (theChangedProvider && m_BreadCrumbProvider)
        QObject::disconnect(m_BreadCrumbProvider, &IBreadCrumbProvider::SigBreadCrumbUpdate, 0, 0);

    m_BreadCrumbProvider = inBreadCrumbProvider;

    SuspendRecalcLayout(true);
    size_t theIndex = 0;

    if (m_BreadCrumbProvider) {
        // listen for single breadcrumb update (i.e. name changes)
        if (theChangedProvider) {
            QObject::connect(m_BreadCrumbProvider, &IBreadCrumbProvider::SigBreadCrumbUpdate,
                             std::bind(&CBreadCrumbControl::OnUpdateBreadCrumb, this));
        }

        const IBreadCrumbProvider::TTrailList &theList = m_BreadCrumbProvider->GetTrail();
        // By design, if this is only 1 item in the list, nothing is shown.
        if (theList.size() > 1) {
            for (; theIndex < theList.size(); ++theIndex) {
                TButtonType *theButton = nullptr;
                if (theIndex < m_BreadCrumbList.size())
                    theButton = m_BreadCrumbList[theIndex].m_BreadCrumb;
                else // create new
                {
                    theButton =
                            CreateButton((theIndex == 0) ? m_BreadCrumbProvider->GetRootImage()
                                                         : m_BreadCrumbProvider->GetBreadCrumbImage());
                    QObject::connect(theButton,&CToggleButton::SigToggle,
                                     std::bind(&CBreadCrumbControl::OnButtonToggled, this,
                                               std::placeholders::_1, std::placeholders::_2));

                    TSeparatorButtonType *theSeparator =
                            CreateSeparatorButton(m_BreadCrumbProvider->GetSeparatorImage());

                    // the ":" is always added as a pair
                    m_BreadCrumbList.push_back(SBreadCrumbItem(theButton, theSeparator));
                    AddChild(theButton);
                    AddChild(theSeparator);
                }
                // Update text and color
                GenerateButtonText(theList[theIndex], theButton);

                // Refresh all properties that might be outdated since buttons are reused
                QPixmap theImage;
                bool theEnabled = true;
                if (theIndex
                        == theList.size()
                        - 1) { // The last button is displayed differently (grayed-out)
                    theButton->SetTextColorUp(CColor(94, 91, 85));
                    theButton->SetTextColorDown(CColor(94, 91, 85));
                    theImage = m_BreadCrumbProvider->GetActiveBreadCrumbImage();

                    theEnabled = false; // the last item being the 'active' is disabled.
                } else if (theIndex > 0)
                    theImage = m_BreadCrumbProvider->GetBreadCrumbImage();

                if (!theImage.isNull())
                    theButton->SetUpImage(theImage);

                theButton->SetToggleState(!theEnabled);
                theButton->SetEnabled(theEnabled);
                // the separator is hidden for the last one
                m_BreadCrumbList[theIndex].m_Separator->SetVisible(theEnabled);

                theButton->Invalidate();
            }
        }
    }
    // Discard 'extras' OR if this is no provider, this will remove all breadcrumbs
    while (m_BreadCrumbList.size() > theIndex) {
        RemoveButton(m_BreadCrumbList.back());
        m_BreadCrumbList.pop_back();
    }
    SuspendRecalcLayout(false);
    RecalcLayout();
}

//=============================================================================
/**
 *	Check for changes in the existing list and update.
 */
void CBreadCrumbControl::OnUpdateBreadCrumb()
{
    ASSERT(m_BreadCrumbProvider);
    bool theUpdated = false;

    const IBreadCrumbProvider::TTrailList &theList = m_BreadCrumbProvider->GetTrail(false);
    ASSERT(m_BreadCrumbList.size() == theList.size()
           || (m_BreadCrumbList.size() == 0
               && theList.size()
               == 1)); // By design, if this is only 1 item in the list, nothing is shown.

    for (size_t theIndex = 0; theIndex < m_BreadCrumbList.size(); ++theIndex) {
        TButtonType *theButton = m_BreadCrumbList[theIndex].m_BreadCrumb;
        if (theButton->GetText() != theList[theIndex].m_String
                || theButton->GetTextColorUp().getQColor() != theList[theIndex].m_Color) {
            GenerateButtonText(theList[theIndex], theButton);
            theUpdated = true;
        }
    }

    if (theUpdated) // Make sure that this entire BreadCrumbControl has room for extraneous length
        // buttons
        RecalcLayout();
}

//=============================================================================
/**
 * Removes and clean up button
 */
void CBreadCrumbControl::RemoveButton(SBreadCrumbItem &inBreadCrumb)
{
    QObject::disconnect(inBreadCrumb.m_BreadCrumb, &CToggleButton::SigToggle, 0, 0);
    RemoveChild(inBreadCrumb.m_BreadCrumb);
    RemoveChild(inBreadCrumb.m_Separator);
    delete inBreadCrumb.m_BreadCrumb;
    delete inBreadCrumb.m_Separator;
}

//=============================================================================
/**
 * Removes all buttons (trail and separators)
 */
void CBreadCrumbControl::RemoveAllButtons()
{
    while (!m_BreadCrumbList.empty()) {
        RemoveButton(m_BreadCrumbList.back());
        m_BreadCrumbList.pop_back();
    }
}

//=============================================================================
/**
 * Handles turning a filter on or off in response to a button being pressed.
 * @param inButton button that generated the event
 * @param inState new state of the button after being toggled
 */
void CBreadCrumbControl::OnButtonToggled(CToggleButton *inButton,
                                         CButtonControl::EButtonState inState)
{
    Q_UNUSED(inState);

    ASSERT(m_BreadCrumbProvider);

    // Find the index into the trail list
    size_t theIndex = 0;
    for (; theIndex < m_BreadCrumbList.size(); ++theIndex) {
        if (m_BreadCrumbList[theIndex].m_BreadCrumb == inButton)
            break;
    }
    ASSERT(theIndex < m_BreadCrumbList.size()); // sanity check

    m_BreadCrumbProvider->OnBreadCrumbClicked((long)theIndex);
}
