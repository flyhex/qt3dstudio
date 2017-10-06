/****************************************************************************
**
** Copyright (C) 2002-2003 NVIDIA Corporation.
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

#ifndef INCLUDED_PLAYBACK_CLOCK
#define INCLUDED_PLAYBACK_CLOCK 1

//==============================================================================
//	Includes
//==============================================================================
#include "UICDMHandles.h"
#include "Timer.h"

class CDoc;
class CClientDataModelBridge;

namespace UICDM {
class ISlideSystem;
class IPropertySystem;
}

struct PlaybackClockPolicy
{
    enum Enum {
        Normal,
        RoundTrip,
    };
};

//==============================================================================
/**
 *	@class CPlaybackClock
 */
class CPlaybackClock
{
    // Field Variables
protected:
    PlaybackClockPolicy::Enum m_Policy;
    bool m_Looping;
    long m_UpperBound;

    CDoc *m_Doc;
    long m_LastAbsoluteTime;
    long m_VirtualTime;

    CTimer m_ContextTimer;

    // Constructors
public:
    CPlaybackClock(CDoc *inDoc);
    virtual ~CPlaybackClock();

public:
    virtual void UpdateTime();

    // Playback Related
    void StartPlayback();
    void StopPlayback();
    void Reset();
    void OnTimeChange(long inNewTime);

    // Implementation
protected:
    void UpdateClockProperties();
    void OnReachedUpperBound();

    // UICDM Helper functions.
    inline UICDM::IPropertySystem *GetPropertySystem() const;
    inline UICDM::ISlideSystem *GetSlideSystem() const;
    inline CClientDataModelBridge *GetClientDataModelBridge() const;
};

#endif