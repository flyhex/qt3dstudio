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
#ifndef __IINSPECTABLEITEM_H__
#define __IINSPECTABLEITEM_H__

//==============================================================================
//	Includes
//==============================================================================
#include "UICDMDataTypes.h"
#include "UICDMHandles.h"
#include "UICDMActionInfo.h"
#include "UICDMMetaData.h"
#include "UICString.h"

//==============================================================================
//	Forwards
//==============================================================================
class CStudioApp;
class IInspectableItem;

//==============================================================================
//	Abstract Base Classes
//==============================================================================

enum EInspectableItemTypes {
    INSPECTABLEITEMTYPE_VANILLA = 1,
    INSPECTABLEITEMTYPE_PROPERTY,
    INSPECTABLEITEMTYPE_DEPENDENT,
    INSPECTABLEITEMTYPE_SLIDE,
    INSPECTABLEITEMTYPE_OBJECTREFERENCE,
    INSPECTABLEITEMTYPE_EVENTSOURCE,
    INSPECTABLEITEMTYPE_ACTION,
    INSPECTABLEITEMTYPE_CONDITIONS,
};

//==============================================================================
/**
 *	@class	IInspectableItemChangeListener
 *	@brief	Listener class for inspectable item changes.
 */
class IInspectableItemChangeListener
{
public:
    virtual void OnInspectablePropertyChanged(IInspectableItem *inProperty) = 0;
};

class IInspectableObject
{
public:
    virtual UICDM::CUICDMInstanceHandle GetInspectableBaseInstance() = 0;
    virtual void SetInspectableObject(const UICDM::SObjectRefType &) = 0;
    virtual UICDM::SObjectRefType GetInspectableObject() = 0;
};

class IInspectableEvent
{
public:
    virtual UICDM::CUICDMInstanceHandle GetInspectableInstance() = 0;
    virtual UICDM::CUICDMEventHandle GetInspectableEvent() = 0;
    virtual void SetInspectableEvent(const UICDM::CUICDMEventHandle &inEventHandle) = 0;
};

class IInspectableTargetSection : public IInspectableObject
{
public:
    virtual UICDM::CUICDMActionHandle GetInspectableAction() const = 0;
};

class IInspectableEventSection : public IInspectableObject, public IInspectableEvent
{
public:
    virtual UICDM::CUICDMActionHandle GetInspectableAction() const = 0;
};

class IInspectableHandlerSection
{
public:
    virtual UICDM::CUICDMActionHandle GetInspectableAction() const = 0;
    virtual UICDM::CUICDMHandlerHandle GetInspectableHandler() = 0;
    virtual void SetInspectableHandler(const UICDM::CUICDMHandlerHandle &inHandlerHandle) = 0;

    virtual UICDM::THandlerHandleList GetInspectableHandlerList() = 0;
    virtual long GetArgumentCount() = 0;
    virtual IInspectableItem *GetArgument(long inIndex) = 0;
    virtual Q3DStudio::CString GetInspectableDescription() = 0;
};

//==============================================================================
/**
 *	@class	IInspectableItem
 *	@brief	Abstract base class for inspectable items.
 */
class IInspectableItem
{
public:
    virtual ~IInspectableItem() {}
    virtual EInspectableItemTypes GetInspectableKind() { return INSPECTABLEITEMTYPE_VANILLA; }

    virtual UICDM::HandlerArgumentType::Value
    GetInspectableSubType() const = 0; // TODO : Make this method name correct
    virtual Q3DStudio::CString GetInspectableName() const = 0;
    virtual Q3DStudio::CString GetInspectableFormalName() const = 0;
    virtual Q3DStudio::CString GetInspectableDescription() const = 0;

    virtual UICDM::SValue GetInspectableData() const = 0;
    virtual void SetInspectableData(const UICDM::SValue &) = 0;

    // TODO: Remove from here onwards after cleaning up the rest of the UI classes
    // This is the non-commital version of SetInspectableData, which must be called
    // after ChangeInspectableData to commit the action.
    virtual bool GetInspectableReadOnly() const { return false; }

    virtual void ChangeInspectableData(const UICDM::SValue & /*inAttr*/){};
    virtual void CancelInspectableData(){}

    virtual void AddInspectableChangeListener(IInspectableItemChangeListener * /*inListener*/){};
    virtual void RemoveInspectableChangeListener(IInspectableItemChangeListener * /*inListener*/){};
};

//==============================================================================
/**
 *	Property specialization
 */
class IInspectablePropertyItem : public IInspectableItem
{
public:
    EInspectableItemTypes GetInspectableKind() override { return INSPECTABLEITEMTYPE_PROPERTY; }
    virtual void GetInspectablePropertyList(UICDM::TPropertyHandleList &outList) = 0;
    virtual UICDM::CUICDMInstanceHandle GetInspectableInstance() = 0;
};

//==============================================================================
/**
 *	Attribute specialization
 */
class IInspectableAttributeItem : public IInspectableItem
{
public:
    EInspectableItemTypes GetInspectableKind() override { return INSPECTABLEITEMTYPE_DEPENDENT; }
    virtual float GetInspectableMin() const = 0;
    virtual float GetInspectableMax() const = 0;
    virtual UICDM::TMetaDataStringList GetInspectableList() const = 0;
    virtual UICDM::DataModelDataType::Value GetInspectableType() const = 0;
    virtual UICDM::AdditionalMetaDataType::Value GetInspectableAdditionalType() const = 0;
};

//==============================================================================
/**
 *	Slide specialization
 */
class IInspectableSlideItem : public IInspectableItem
{
public:
    EInspectableItemTypes GetInspectableKind() override { return INSPECTABLEITEMTYPE_SLIDE; }
    virtual void GetSlideNames(std::list<Q3DStudio::CString> &outSlideNames) = 0;
};

//==============================================================================
/**
 *	ObjectReference specialiaztion
 */
class IInspectableObjectRefItem : public IInspectableObject, public IInspectableItem
{
public:
    EInspectableItemTypes GetInspectableKind() override
    {
        return INSPECTABLEITEMTYPE_OBJECTREFERENCE;
    }
};

//==============================================================================
/**
 *	Event specialization
 */
class IInspectableEventItem : public IInspectableEvent, public IInspectableItem
{
public:
    EInspectableItemTypes GetInspectableKind() override { return INSPECTABLEITEMTYPE_EVENTSOURCE; }
};

#endif // #ifndef __IINSPECTABLEITEM_H__
