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

#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "MemoryObject.h"
#include "Qt3DSString.h"
#include "Qt3DSFunctor.h"
#include "Multicaster.h"

//==============================================================================
//	CMemoryObjectDetail
//==============================================================================

/**
 *	Constructor
 */
CMemoryObjectDetail::CMemoryObjectDetail()
{
    Clear();
}

/**
 *	Add some memory detail
 */
void CMemoryObjectDetail::Add(const char *inName, long inSize)
{
    m_DetailList.push_back(std::make_pair(inName, inSize));
    m_TotalSize += inSize;
}

/**
 *	Clear the detail list
 */
void CMemoryObjectDetail::Clear()
{
    m_TotalSize = 0;
    m_DetailList.clear();
}

//==============================================================================
//	CMemoryObject
//==============================================================================

long CMemoryObject::s_MemoryObjectID = 0; ///< static variable used for generating unique id's

/**
 *	Constructor
 */
CMemoryObject::CMemoryObject(const char *inName, long inStaticSize,
                             Q3DStudio::IMemoryObject *inMemoryObject)
    : m_ID(++s_MemoryObjectID)
    , m_StaticSize(inStaticSize)
    , m_Name(inName)
    , m_MemoryObject(inMemoryObject)
    , m_MemoryObjectParent(NULL)
    , m_SizeFunctor(NULL)
{
}

/**
 *	Copy Constructor
 */
CMemoryObject::CMemoryObject(const CMemoryObject &inSource,
                             Q3DStudio::IMemoryObject *inMemoryObject)
    : m_ID(++s_MemoryObjectID)
    , m_MemoryObjectParent(NULL)
    , m_SizeFunctor(NULL)
{
    m_Name = inSource.m_Name;
    m_StaticSize = inSource.m_StaticSize;
    m_MemoryObject = inMemoryObject;
}

/**
 *	Destructor
 */
CMemoryObject::~CMemoryObject()
{
    if (m_MemoryObjectParent && m_MemoryObject)
        m_MemoryObjectParent->UnregisterMemoryObject(m_MemoryObject);
    else if (m_MemoryObject)
        m_ChangeListener.FireEvent(&Q3DStudio::IMemoryObjectListener::OnMemoryObjectUnRegistered,
                                   m_MemoryObject);

    Q3DStudio::IMemoryObject::TObjectList::iterator theIterator = m_ObjectList.begin();
    for (; theIterator < m_ObjectList.end(); ++theIterator)
        (*theIterator)->SetMemoryObjectParent(NULL);
}

/**
 *	This functor is called to determine the size details of an object.
 *
 *	This allows any class the uses CMemoryObject to set a functor to an
 *	arbitrary function like this:
 *
 *	<pre>
 *	SetSizeFunctor( new CFunctorObjectParam1<CClass,CMemoryObjectDetail&>( this,
 *CClass::GetMemoryUsage ) );
 *	</pre>
 */
void CMemoryObject::SetSizeFunctor(TSizeFunctor *inFunctor)
{
    m_SizeFunctor = inFunctor;
}

/**
 *	Call the size functor if it has been set for the object
 */
long CMemoryObject::CallSizeFunctor() const
{
    m_ObjectDetails.Clear();

    // TODO: Calculate internal memory tracking usage

    if (m_SizeFunctor)
        m_SizeFunctor->Execute(m_ObjectDetails);

    return m_ObjectDetails.m_TotalSize;
}

/**
 *	This functor is called to determine the description details of an object.
 *
 *	This allows any class the uses CMemoryObject to set a functor to an
 *	arbitrary function like this:
 *
 *	<pre>
 *	SetDescFunctor( new CFunctorObjectParam1<CClass,Q3DStudio::CString&>( this,
 *CClass::GetMemeoryDescription ) );
 *	</pre>
 */
void CMemoryObject::SetDescFunctor(TDescFunctor *inFunctor)
{
    m_DescFunctor = inFunctor;
}

/**
 *	Call the desc functor if it has been set for the object
 */
Q3DStudio::CString CMemoryObject::CallDescFunctor() const
{
    Q3DStudio::CString theDesc;

    if (m_DescFunctor)
        m_DescFunctor->Execute(theDesc);

    return theDesc;
}

//==============================================================================
// IMemoryObject
//==============================================================================

/**
 *	Return the name of the memory object
 */
const char *CMemoryObject::GetMemoryObjectName() const
{
    return m_Name;
}

/**
 *	Return a description of the memory object
 */
const char *CMemoryObject::GetMemoryObjectDescription() const
{
    m_Desc = CallDescFunctor();
    return (const char *)m_Desc.GetCharStar();
}

/**
 *	Return a unique identifier for the memory object
 */
long CMemoryObject::GetMemoryObjectId() const
{
    return m_ID;
}

/**
 *	Locate the specified unique identifier in the IMemoryObject hierarchy.
 */
Q3DStudio::IMemoryObject *CMemoryObject::FindMemoryObjectId(long inId) const
{
    Q3DStudio::IMemoryObject *theMemoryObject = NULL;
    Q3DStudio::IMemoryObject::TObjectList::const_iterator theIterator = m_ObjectList.begin();
    for (; theIterator < m_ObjectList.end(); ++theIterator) {
        if ((*theIterator)->GetMemoryObjectId() == inId)
            return *theIterator;

        theMemoryObject = (*theIterator)->FindMemoryObjectId(inId);
        if (theMemoryObject)
            return theMemoryObject;
    }

    return NULL;
}

/**
 *	Register a child memory object that is "owned" by this object.
 */
void CMemoryObject::RegisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker)
{
    m_ObjectList.push_back(inMemoryTracker);

    inMemoryTracker->SetMemoryObjectParent(m_MemoryObject);

    m_ChangeListener.FireEvent(&Q3DStudio::IMemoryObjectListener::OnMemoryObjectRegistered,
                               inMemoryTracker);
}

/**
 *	Unregister a child memory object that is "owned" by this object.
 */
void CMemoryObject::UnregisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker)
{
    m_ChangeListener.FireEvent(&Q3DStudio::IMemoryObjectListener::OnMemoryObjectUnRegistered,
                               inMemoryTracker);

    inMemoryTracker->SetMemoryObjectParent(NULL);

    Q3DStudio::IMemoryObject::TObjectList::iterator theIterator = m_ObjectList.begin();
    for (; theIterator < m_ObjectList.end(); ++theIterator) {
        if (inMemoryTracker == *theIterator) {
            m_ObjectList.erase(theIterator);
            return;
        }
    }
}

/**
 *	Register a memory object listener to listen for change events.
 */
void CMemoryObject::RegisterMemoryObjectListener(
    Q3DStudio::IMemoryObjectListener *inMemoryObjectListener)
{
    m_ChangeListener.AddListener(inMemoryObjectListener);
}

/**
 *	Unregister a memory object listener to listen for change events.
 */
void CMemoryObject::UnregisterMemoryObjectListener(
    Q3DStudio::IMemoryObjectListener *inMemoryObjectListener)
{
    m_ChangeListener.RemoveListener(inMemoryObjectListener);
}

/**
 *	Returns the size of the base object (sizeof)
 */
long CMemoryObject::GetMemoryObjectBase() const
{
    return m_StaticSize;
}

/**
 *	Returns the size of the base object (sizeof) and any dynamic memory
 */
long CMemoryObject::GetMemoryObjectUsage() const
{
    long theMemoryUsage = GetMemoryObjectBase();
    theMemoryUsage += CallSizeFunctor();
    return theMemoryUsage;
}

/**
 *	Returns the size of the base object (sizeof), any dynamic memory and the total size of any
 *children
 */
long CMemoryObject::GetMemoryObjectTotal() const
{
    long theMemoryUsage = m_StaticSize;

    theMemoryUsage += CallSizeFunctor();

    Q3DStudio::IMemoryObject::TObjectListIt theIterator = m_ObjectList.begin();
    for (; theIterator != m_ObjectList.end(); ++theIterator) {
        Q3DStudio::IMemoryObject *theMemoryObject = *theIterator;
        theMemoryUsage += theMemoryObject->GetMemoryObjectTotal();
    }

    return theMemoryUsage;
}

/**
 *	Iterate through child memory objects
 */
const Q3DStudio::IMemoryObject::TObjectListIt CMemoryObject::GetMemoryObjectBegin() const
{
    return m_ObjectList.begin();
}

/**
 *	Iterate through child memory objects
 */
const Q3DStudio::IMemoryObject::TObjectListIt CMemoryObject::GetMemoryObjectEnd() const
{
    return m_ObjectList.end();
}

/**
 *	Iterate through the details of the memory objects dynamic memory
 */
const Q3DStudio::IMemoryObject::TDetailListIt CMemoryObject::GetMemoryObjectDetailBegin() const
{
    return m_ObjectDetails.m_DetailList.begin();
}

/**
 *	Iterate through the details of the memory objects dynamic memory
 */
const Q3DStudio::IMemoryObject::TDetailListIt CMemoryObject::GetMemoryObjectDetailEnd() const
{
    return m_ObjectDetails.m_DetailList.end();
}

/**
 *	Store the parent of the object so that it can be notified when the object is destroyed
 */
void CMemoryObject::SetMemoryObjectParent(Q3DStudio::IMemoryObject *inMemoryObject)
{
    m_MemoryObjectParent = inMemoryObject;
}
