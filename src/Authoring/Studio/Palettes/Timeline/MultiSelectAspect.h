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

#ifndef INCLUDED_MULTISELECT_ASPECT_H
#define INCLUDED_MULTISELECT_ASPECT_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "StateRow.h"
#include "Bindings/IKeyframeSelector.h"

//==============================================================================
/**
 * This class handles the multi selection aspect for keyframes in the timeline.
 * It toggles each keyframe's select state on or off, depending on whether the
 * mouse rectangle is over them and if the modifier key is down.
 */
template <typename TSTLKeyFrameList>
class CMultiSelectAspect
{

public:
    typedef typename TSTLKeyFrameList::iterator TSTLKeyframesItr;

protected:
    TSTLKeyFrameList &m_STLKeyframes; ///< stores list of keyframes in a vector (STL)
    IKeyframeSelector *m_KeyframeSelector; ///< the interface that performs the keyframes selection

public:
    //=============================================================================
    /**
    * Constructor
    * @param inSTLKeyframes stores a list of keyframes in a vector
    * @param inDoc stores the studio document
    */
    CMultiSelectAspect(TSTLKeyFrameList &inSTLKeyframes, IKeyframeSelector *inKeyframeSelector)
        : m_STLKeyframes(inSTLKeyframes)
        , m_KeyframeSelector(inKeyframeSelector)
    {
    }

    //=============================================================================
    /**
    * Destructor
    */
    ~CMultiSelectAspect() {}

    //=============================================================================
    /**
    * CommitSelections: Overwrites all previous keyframe states with the current keyframe states.
    *					This will prevent the keyframes in the current selection
*from
*					switching states as we select other keyframes.
    */
    void CommitSelections()
    {
        // Iterates each keyframe and set the previous state to the current one.
        TSTLKeyframesItr thePos = m_STLKeyframes.begin();
        for (; thePos != m_STLKeyframes.end(); ++thePos) {
            (*thePos)->SetPreviousSelectState((*thePos)->IsSelected());
        }
    }

    //=============================================================================
    /**
    * MultiSelect: Handles the selection of keyframes in a given rect.
    * @param inRect stores the rectangle that will be used to select the keyframes within it.
    * @param inModifierKeyDown indicates if thte modifier key is pressed.
    */
    void MultiSelect(CRct inRect, bool inModifierKeyDown)
    {
        // Iterates through the keyframes and checks if the keys are in the rect and
        // perform the necessary selection operations on each keyframe.
        TSTLKeyframesItr thePos = m_STLKeyframes.begin();
        for (; thePos != m_STLKeyframes.end(); ++thePos) {
            bool isInRect = (*thePos)->IsInRect(inRect);
            if ((*thePos)->IsEnabled()) {
                if (inModifierKeyDown) {
                    if (isInRect) {
                        if (!(*thePos)->GetRectOverHandled()) {
                            bool theSelectState = (*thePos)->IsSelected();

                            // Update the previous select state
                            (*thePos)->SetPreviousSelectState(theSelectState);
                            // Negate the keyframe state when it is in the rectangle
                            theSelectState = !theSelectState;
                            (*thePos)->Select(theSelectState);
                            m_KeyframeSelector->SelectKeyframes(theSelectState,
                                                                (*thePos)->GetTime());

                            // Set the RectOverFlag to true, so that we will not repeat the negation
                            // in indefinitely.
                            (*thePos)->SetRectOverHandled(true);
                        }
                    } else {
                        // When the rectangle is not over the current keyframe, revert its state to
                        // the previous one
                        if ((*thePos)->IsSelected() != (*thePos)->GetPreviousSelectState()) {
                            (*thePos)->Select((*thePos)->GetPreviousSelectState());
                            m_KeyframeSelector->SelectKeyframes((*thePos)->GetPreviousSelectState(),
                                                                (*thePos)->GetTime());
                        }
                        (*thePos)->SetRectOverHandled(false);
                    }
                } else {
                    // When modifier is not pressed we will just select the keyframes if it is over
                    // in the rectangle and deselect it when it isn't.
                    if ((*thePos)->IsSelected() != isInRect) {
                        (*thePos)->SetPreviousSelectState((*thePos)->IsSelected());
                        m_KeyframeSelector->SelectKeyframes(isInRect, (*thePos)->GetTime());
                        (*thePos)->Select(isInRect);
                    }
                    (*thePos)->SetRectOverHandled(false);
                }
            }
        }
    }
};
#endif // INCLUDED_MULTISELECT_ASPECT_H
