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
#ifndef INCLUDED_TIMELINE_CONTROL_H
#define INCLUDED_TIMELINE_CONTROL_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "DispatchListeners.h"
#include "TimelineRow.h"
#include "ITimelineControl.h"

#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSignals.h"
#include "SelectedValueImpl.h"

//==============================================================================
//	Forwards
//==============================================================================
class CDoc;
class CDropTarget;
class CTimelineSplitter;
class CTimelineTreeLayout;
class CTimelineTimelineLayout;
class CScroller;
class CInsertionLine;
class CInsertionOverlay;
class CRenderer;
class CHotKeys;
class CBreadCrumbControl;
class CTimelineTranslationManager;
class CClientDataModelBridge;

//==============================================================================
//	Classes
//==============================================================================

class CTimelineControl : public CControl,
                         public CPresentationChangeListener,
                         public CClientPlayChangeListener,
                         public ITimelineControl
{
public:
    CTimelineControl();
    ~CTimelineControl();

    DEFINE_OBJECT_COUNTER(CTimelineControl)

    // CControl
    void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation = false) override;
    void Draw(CRenderer *inRenderer) override;
    void OnGainFocus() override;
    CDropTarget *FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    // Presentation Change Listener
    void OnNewPresentation() override;
    void OnClosingPresentation() override;
    void OnSavingPresentation(const Qt3DSFile *inNewPresentationFile) override;

    // ClientPlayChangeListener
    void OnPlayStart() override;
    void OnPlayStop() override;
    void OnTimeChanged(long inNewTime) override;

    // CSelectionChangeListener,
    virtual void OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable);

    // ITimelineControl
    void OnLayoutChanged() override;
    CRct GetBounds() const override;
    void HideTimelineMoveableTooltip() override;
    ISnappingListProvider *GetSnappingListProvider() const override;

    void ClearView();
    void ViewSlide(qt3dsdm::Qt3DSDMSlideHandle inSlide);
    qt3dsdm::Qt3DSDMSlideHandle GetActiveSlide();

    CTimelineTimelineLayout *GetTimelineLayout();
    CTimelineTreeLayout *GetTreeLayout();

    void SetSize(CPt inSize) override;
    long GetTime();
    void SetScrollPositionY(CScroller *inSource, long inPositionY);

    CInsertionLine *GetInsertionLine();
    CInsertionOverlay *GetInsertionOverlay();
    CRct GetVisibleTreeLayoutArea();

    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler);

    CTimelineTranslationManager *GetTranslationManager() const { return m_TranslationManager; }

protected:
    void SuspendLayoutChanges(bool inSuspend);
    void SetTime(long inNewTime);

    void HideInsertionMarkers();

    // DataModel callbacks
    void OnActiveSlide(qt3dsdm::Qt3DSDMSlideHandle inSlide);
    void OnDeleteSlide(qt3dsdm::Qt3DSDMSlideHandle inSlide);

    // Helper functions
    inline CDoc *GetDoc();
    inline CClientDataModelBridge *GetBridge();

    CTimelineSplitter *m_Splitter;
    CTimelineTreeLayout *m_TreeLayout;
    CTimelineTimelineLayout *m_TimelineLayout;
    qt3dsdm::Qt3DSDMSlideHandle m_ActiveSlide;
    CInsertionLine
        *m_InsertionLine; ///< Drag-and-drop insertion line for dropping between timeline items
    CInsertionOverlay
        *m_InsertionOverlay; ///< Drag-and-drop insertion marker for dropping on a timeline item
    CBreadCrumbControl *m_BreadCrumbToolbar;
    bool m_SuspendRecalcLayout;

    CTimelineTranslationManager *m_TranslationManager;

    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>>
        m_Connections; /// connections to the DataModel
};
#endif // INCLUDED_TIMELINE_CONTROL_H
