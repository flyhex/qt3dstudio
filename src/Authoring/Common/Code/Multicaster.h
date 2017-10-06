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

#ifndef INCLUDE_MULTICASTER_H
#define INCLUDE_MULTICASTER_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "SafeArray.h"

//==============================================================================
/**
 *	Template class for maintaining a list of listeners and fire events off to them.
 */
template <class TObject>
class CMulticaster
{
    typedef CSafeArray<TObject> TList;

public:
    CMulticaster() {}
    virtual ~CMulticaster() {}

    //==============================================================================
    /**
     *	Adds a listener to the internally maintained list.  First removes any
     *	previous instances of the listener before adding a new one.
     *
     *	@param inListener The listener to be registered.
     */
    void AddListener(TObject inListener)
    {
        this->RemoveListener(inListener);
        m_List.Add(inListener);
    }

    //==============================================================================
    /**
     *	Removes all instances of the specified listener from the registration list.
     *
     *	@param inListener The listener to be unregistered
     */
    void RemoveListener(TObject inListener)
    {
        typename TList::CIterator thePos = m_List.Begin();

        for (; thePos.HasNext(); ++thePos) {
            if (inListener == (*thePos)) {
                m_List.Remove(thePos);
                break;
            }
        }
    }

    //==============================================================================
    /**
     *	Template methods for firing events with arbitrary numbers of params to
     *	listeners.  If a source is specfied, then the source will not receive
     *	notifications that it kicked off itself.
     */
    template <typename TMethodType>
    void FireEvent(TMethodType inMethod)
    {
        this->FireEventSource(inMethod, NULL);
    }

    template <typename TMethodType>
    void FireEventSource(TMethodType inMethod, void *inSource)
    {
        typename TList::CIterator thePos = m_List.Begin();

        for (; thePos.HasNext(); ++thePos) {
            if (*thePos != inSource) {
                ((*thePos)->*inMethod)();
            }
        }
    }

    template <typename TMethodType, typename TFirstArg>
    void FireEvent(TMethodType inMethod, TFirstArg inFirstArg)
    {
        this->FireEventSource(inMethod, inFirstArg, NULL);
    }

    template <typename TMethodType, typename TFirstArg>
    void FireEventSource(TMethodType inMethod, TFirstArg inFirstArg, void *inSource)
    {
        typename TList::CIterator thePos = m_List.Begin();

        for (; thePos.HasNext(); ++thePos) {
            if (*thePos != inSource) {
                ((*thePos)->*inMethod)(inFirstArg);
            }
        }
    }

    template <typename TMethodType, typename TFirstArg, typename TSecondArg>
    void FireEvent(TMethodType inMethod, TFirstArg inFirstArg, TSecondArg inSecondArg)
    {
        this->FireEventSource(inMethod, inFirstArg, inSecondArg, NULL);
    }

    template <typename TMethodType, typename TFirstArg, typename TSecondArg>
    void FireEventSource(TMethodType inMethod, TFirstArg inFirstArg, TSecondArg inSecondArg,
                         void *inSource)
    {
        typename TList::CIterator thePos = m_List.Begin();

        for (; thePos.HasNext(); ++thePos) {
            if (*thePos != inSource) {
                ((*thePos)->*inMethod)(inFirstArg, inSecondArg);
            }
        }
    }

    template <typename TMethodType, typename TFirstArg, typename TSecondArg, typename TThirdArg>
    void FireEvent(TMethodType inMethod, TFirstArg inFirstArg, TSecondArg inSecondArg,
                   TThirdArg inThirdArg)
    {
        this->FireEventSource(inMethod, inFirstArg, inSecondArg, inThirdArg, NULL);
    }

    template <typename TMethodType, typename TFirstArg, typename TSecondArg, typename TThirdArg>
    void FireEventSource(TMethodType inMethod, TFirstArg inFirstArg, TSecondArg inSecondArg,
                         TThirdArg inThirdArg, void *inSource)
    {
        typename TList::CIterator thePos = m_List.Begin();

        for (; thePos.HasNext(); ++thePos) {
            if (*thePos != inSource) {
                ((*thePos)->*inMethod)(inFirstArg, inSecondArg, inThirdArg);
            }
        }
    }

    template <typename TMethodType, typename TFirstArg, typename TSecondArg, typename TThirdArg,
              typename TFourthArg>
    void FireEvent(TMethodType inMethod, TFirstArg inFirstArg, TSecondArg inSecondArg,
                   TThirdArg inThirdArg, TFourthArg inFourthArg)
    {
        this->FireEventSource(inMethod, inFirstArg, inSecondArg, inThirdArg, inFourthArg, NULL);
    }

    template <typename TMethodType, typename TFirstArg, typename TSecondArg, typename TThirdArg,
              typename TFourthArg>
    void FireEventSource(TMethodType inMethod, TFirstArg inFirstArg, TSecondArg inSecondArg,
                         TThirdArg inThirdArg, TFourthArg inFourthArg, void *inSource)
    {
        typename TList::CIterator thePos = m_List.Begin();

        for (; thePos.HasNext(); ++thePos) {
            if (*thePos != inSource) {
                ((*thePos)->*inMethod)(inFirstArg, inSecondArg, inThirdArg, inFourthArg);
            }
        }
    }

    //=========================================================================
    /**
     * Get the number of listeners in this multicaster.
     * @return the number of listeners in this multicaster.
     */
    long GetCount() { return m_List.GetCount(); }

protected:
    TList m_List; ///< List of objects to fire events to
};

#endif // INCLUDE_MULTICASTER_H
