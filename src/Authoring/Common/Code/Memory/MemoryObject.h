/****************************************************************************
**
** Copyright (C) 1999-2005 NVIDIA Corporation.
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

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "IMemoryObject.h"
#include "UICFunctor.h"
#include "Multicaster.h"

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
//	Classes
//==============================================================================

//==============================================================================
/**
 *	This is a helper class for tracking memory detail on an object.
 */
class CMemoryObjectDetail
{
    // Typedefs
public:
    Q3DStudio::IMemoryObject::TDetailList m_DetailList; ///< the list of object details

    // Field Variables
    long m_TotalSize;

    // Constructor
public:
    CMemoryObjectDetail();

    // Implementation
public:
    void Add(const char *inName, long inSize);
    void Clear();
};

//==============================================================================
/**
 *	This is a helper class for tracking memory usage on an object.
 *
 *	This class is a little odd, but here are the reasons it was implemented the
 *	way it is.
 *
 *	Initially, this was a template class that derived from IMemoryObject, but
 *	this caused a problem with CAsset type objects, since the CAsset needed to
 *	derive from IMemoryObject, but the implementation was on the instance level,
 *	such as CCamera. This could be addressed, but then there were issues with
 *	classes such as CPropertyContainer, both of which needed to be IMemoryObject
 *	derived classes, but CPropertyContainer needed to stand alone.
 *
 *	After a number of implementations, it was determined that the simplest and
 *	most maintainable version involved the MemoryObjectMacros that create a
 *	m_MemoryObject field variable on each class and and pseudo-interface that
 *	redirects call to the m_MemoryObject handler.
 */
class CMemoryObject
{
    // typedef
public:
    typedef Q3DStudio::CFunctorObjectParam1Base<CMemoryObjectDetail &>
        TSizeFunctor; ///< Memory object details
    typedef Q3DStudio::CFunctorObjectParam1Base<Q3DStudio::CString &>
        TDescFunctor; ///< Memory object description

    // Field Variables
protected:
    long m_ID; ///< unique id for the object
    long m_StaticSize; ///< the size of the object, not including dynamic memory allocations
                       ///(usually calculated by using sizeof)
    const char *m_Name; ///< pointer to the name of the object (not an Q3DStudio::CString because it
                        ///has issues sharing memory across DLL boundaries and the gumshoe plugin is
                        ///a DLL)
    mutable Q3DStudio::CString m_Desc; ///< to provide persistance for the description data
    Q3DStudio::IMemoryObject::TObjectList m_ObjectList; ///< the list of child memory objects
    CMulticaster<Q3DStudio::IMemoryObjectListener *> m_ChangeListener; ///< the list of memory
                                                                       ///change listeners (just for
                                                                       ///adding and removing memory
                                                                       ///objects)
    Q3DStudio::IMemoryObject *m_MemoryObject; ///< a pointer to "this" memory object (for
                                              ///auto-unregistering the object on deletion)
    Q3DStudio::IMemoryObject *m_MemoryObjectParent; ///< a pointer to "this" memory objects parent
                                                    ///(for auto-unregistering the object on
                                                    ///deletion)
    Q3DStudio::CAutoMemPtr<TSizeFunctor>
        m_SizeFunctor; ///< the functor to call when dynamic memory usage needs to be calculated
    Q3DStudio::CAutoMemPtr<TDescFunctor>
        m_DescFunctor; ///< the functor to call to grab description information
    mutable CMemoryObjectDetail m_ObjectDetails; ///< the detailed information about the object

    // Static Variables
protected:
    static long s_MemoryObjectID; ///< static variable used for generating unique id's

    // Constructor
public:
    CMemoryObject(const char *inName, long inStaticSize, Q3DStudio::IMemoryObject *inMemoryObject);
    CMemoryObject(const CMemoryObject &inSource, Q3DStudio::IMemoryObject *inMemoryObject);
    ~CMemoryObject();

    void SetSizeFunctor(TSizeFunctor *inFunctor);
    long CallSizeFunctor() const;

    void SetDescFunctor(TDescFunctor *inFunctor);
    Q3DStudio::CString CallDescFunctor() const;

    // Accessors
public:
    // IMemoryObject
public:
    const char *GetMemoryObjectName() const;
    const char *GetMemoryObjectDescription() const;
    long GetMemoryObjectId() const;
    Q3DStudio::IMemoryObject *FindMemoryObjectId(long inId) const;
    void RegisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker);
    void UnregisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker);
    void RegisterMemoryObjectListener(Q3DStudio::IMemoryObjectListener *inMemoryObjectListener);
    void UnregisterMemoryObjectListener(Q3DStudio::IMemoryObjectListener *inMemoryObjectListener);
    long GetMemoryObjectBase() const;
    long GetMemoryObjectUsage() const;
    long GetMemoryObjectTotal() const;
    const Q3DStudio::IMemoryObject::TObjectListIt GetMemoryObjectBegin() const;
    const Q3DStudio::IMemoryObject::TObjectListIt GetMemoryObjectEnd() const;
    const Q3DStudio::IMemoryObject::TDetailListIt GetMemoryObjectDetailBegin() const;
    const Q3DStudio::IMemoryObject::TDetailListIt GetMemoryObjectDetailEnd() const;
    void SetMemoryObjectParent(Q3DStudio::IMemoryObject *inMemoryObject);

    // Static Methods
public:
    // Internal	Methods
protected:
    // Operators
private:
    CMemoryObject &operator=(const CMemoryObject &inSrc);
};
