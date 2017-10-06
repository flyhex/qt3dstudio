/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

#ifndef __IMemoryObject_H_
#define __IMemoryObject_H_

namespace Q3DStudio {

// Forwards
class IMemoryObject;

//==============================================================================
/**
 *	This is an abstract interface for firing memeory object events.
 */
class IMemoryObjectListener
{
    // Interface
public:
    ///> Called when a new memory object is registered
    virtual void OnMemoryObjectRegistered(IMemoryObject *inMemoryObject) = 0;
    ///> Called when a memory object is removed
    virtual void OnMemoryObjectUnRegistered(IMemoryObject *inMemoryObject) = 0;
};

//==============================================================================
/**
 *	This is an abstract interface for traversing hierarchies of memeory objects.
 *
 *	An IMemoryObject maintains information about the memory usage of the object
 *	and a list of child memory objects. Detailed information about object memory
 *	can be exposed by the detail object.
 */
class IMemoryObject
{
    // Typedefs
public:
    typedef std::vector<IMemoryObject *> TObjectList; ///< typedef for child memory objects
    typedef TObjectList::const_iterator
        TObjectListIt; ///< typedef for iterator of  child memory objects

    typedef std::pair<const char *, long> TDetailType; ///< typedef of the detail data
    typedef std::vector<TDetailType> TDetailList; ///< typedef for details of memory object
    typedef TDetailList::const_iterator
        TDetailListIt; ///< typedef for iterator of details of memory object

    // Interface
public:
    ///> Returns the name of the memory object
    virtual const char *GetMemoryObjectName() const = 0;

    ///> Return a description of the object
    virtual const char *GetMemoryObjectDescription() const = 0;

    ///> Returns the unique id of the memory object. This allows another thread (or a UI thread)
    ///> to listen to object deletion events and handle the event in another thread without
    ///> worrying that the pointer to the memory object will go out of scope before the other
    ///> thread has a chance to handle the deletion.
    virtual long GetMemoryObjectId() const = 0;
    virtual IMemoryObject *FindMemoryObjectId(long inId) const = 0;

    ///> Register a child memory object. These are objects that are "owned" by this object
    virtual void RegisterMemoryObject(IMemoryObject *inMemoryObject) = 0;
    virtual void UnregisterMemoryObject(IMemoryObject *inMemoryObject) = 0;

    ///> Register a listener for memory change events
    virtual void RegisterMemoryObjectListener(IMemoryObjectListener *inMemoryObjectListener) = 0;
    virtual void UnregisterMemoryObjectListener(IMemoryObjectListener *inMemoryObjectListener) = 0;

    ///> Returns the size of the base object (sizeof)
    virtual long GetMemoryObjectBase() const = 0;

    ///> Returns the size of the base object (sizeof) and any dynamic memory
    virtual long GetMemoryObjectUsage() const = 0;

    ///> Returns the size of the base object (sizeof), any dynamic memory and the total size of any
    ///children
    virtual long GetMemoryObjectTotal() const = 0;

    ///> Iterate through child memory objects
    virtual const TObjectListIt GetMemoryObjectBegin() const = 0;
    virtual const TObjectListIt GetMemoryObjectEnd() const = 0;

    ///> Iterate through the details of the memory objects dynamic memory
    virtual const TDetailListIt GetMemoryObjectDetailBegin() const = 0;
    virtual const TDetailListIt GetMemoryObjectDetailEnd() const = 0;

    ///> Store the parent of the object so that it can be notified when the object is destroyed
    virtual void SetMemoryObjectParent(IMemoryObject *inMemoryObject) = 0;
};

} // namespace Q3DStudio

#endif //__IMemoryObject_H_
