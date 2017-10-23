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
#ifndef __QT3DS_OBJECTCOUNTER_H__
#define __QT3DS_OBJECTCOUNTER_H__

//==============================================================================
//	Includes
//==============================================================================
#include <vector>
#include <string.h>

//==============================================================================
//	Forwards
//==============================================================================
class Qt3DSObjectCounter;

//==============================================================================
//	Typedefs
//==============================================================================
typedef std::vector<Qt3DSObjectCounter *> TObjectCounterTracker;

//==============================================================================
//	Constants
//==============================================================================

// If defined, track the object counts
#ifdef TRACK_OBJECT_COUNTS

#define DEFINE_OBJECT_COUNTER(classname) static Qt3DSObjectCounter s_ObjectCounter##classname;

#define IMPLEMENT_OBJECT_COUNTER(classname)                                                        \
    Qt3DSObjectCounter classname::s_ObjectCounter##classname(#classname, sizeof(classname));

#define ADDTO_OBJECT_COUNTER(classname) classname::s_ObjectCounter##classname.Add();

#define REMOVEFROM_OBJECT_COUNTER(classname) classname::s_ObjectCounter##classname.Remove();

#define GET_OBJECT_COUNTER_COUNT(classname) (classname::s_ObjectCounter##classname.m_ObjectCount)

#define GET_OBJECT_COUNTER_SIZE(classname) (classname::s_ObjectCounter##classname.m_ObjectSize)

#define DEFINE_OBJECT_COUNTER_TRACKER(tracktype)                                                   \
    extern TObjectCounterTracker s_ObjectCounterTracker##tracktype;

#define IMPLEMENT_OBJECT_COUNTER_TRACKER(tracktype)                                                \
    TObjectCounterTracker s_ObjectCounterTracker##tracktype;

#define TRACK_OBJECT_COUNTER(tracktype, classname)                                                 \
    s_ObjectCounterTracker##tracktype.push_back(&classname::s_ObjectCounter##classname);

#define GET_TRACK_OBJECT_COUNTER(tracktype) s_ObjectCounterTracker##tracktype

// If not defined, do nothing
#else

#define DEFINE_OBJECT_COUNTER(classname)
#define IMPLEMENT_OBJECT_COUNTER(classname)
#define ADDTO_OBJECT_COUNTER(classname)
#define REMOVEFROM_OBJECT_COUNTER(classname)
#define GET_OBJECT_COUNTER_COUNT(classname)
#define DEFINE_OBJECT_COUNTER_TRACKER(tracktype)
#define IMPLEMENT_OBJECT_COUNTER_TRACKER(tracktype)
#define TRACK_OBJECT_COUNTER(tracktype, classname)
#define GET_TRACK_OBJECT_COUNTER(tracktype)

#endif

//==============================================================================
//	Class
//==============================================================================

//==============================================================================
/**
 *	@class	Qt3DSObjectCounter
 *	@brief	This class is used to track that object have been released.
 *
 *	Use the macros to make the feature easy.
 *	The macros define a static variable. Each time the object is constructed,
 *	the count on the static variable in incremented, and decremented when it
 *	is destructed. When the static variable is destroyed, object counts are
 *	dumped out to the debugger.
 */
class Qt3DSObjectCounter
{
    //==============================================================================
    //	Enumerations
    //==============================================================================

    //==============================================================================
    //	Fields
    //==============================================================================

public:
    long m_ObjectCount;
    char m_ObjectName[256];
    long m_ObjectSize;

private:
    static TObjectCounterTracker s_ObjectVector;

    //==============================================================================
    //	Methods
    //==============================================================================

public:
    Qt3DSObjectCounter(const char *inName, long inClassSize)
        : m_ObjectCount(0)
        , m_ObjectSize(inClassSize)
    {
        ::strncpy(m_ObjectName, inName, sizeof(m_ObjectName) - 1);
        m_ObjectName[::strlen(inName)] = 0;

        s_ObjectVector.push_back(this);
    }

    ~Qt3DSObjectCounter()
    {
        if (m_ObjectCount > 0) {
#ifdef WIN32
#ifdef ATLTRACE
            ATLTRACE("\t. . . with %ld unreleased %s objects.\n", m_ObjectCount, m_ObjectName);
#endif
#endif
        }

        else if (m_ObjectCount < 0) {
#ifdef WIN32
#ifdef ATLTRACE
            ATLTRACE("\t. . . object counting error for %s (%ld).\n", m_ObjectName, m_ObjectCount);
#endif
#endif
        }
        if (s_ObjectVector.size()) {
            TObjectCounterTracker::iterator theBegin = s_ObjectVector.begin();
            TObjectCounterTracker::iterator theEnd = s_ObjectVector.end();
            for (; theBegin != theEnd; ++theBegin) {
                if (this == *theBegin) {
                    s_ObjectVector.erase(theBegin);
                    break;
                }
            }
        }
    }
    void Add() { m_ObjectCount++; }
    void Remove() { m_ObjectCount--; };
};

#endif // #ifndef __QT3DS_OBJECTCOUNTER_H__
