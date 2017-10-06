/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#ifndef UICDMDATACOREH
#define UICDMDATACOREH
#include "UICDMHandles.h"
#include "UICDMPropertyDefinition.h"
#include "HandleSystemBase.h"
#include "UICDMMetaData.h"
#include "UICDMValue.h"

namespace UICDM {

typedef std::pair<CUICDMPropertyHandle, SValue> TPropertyHandleValuePair;
typedef std::vector<TPropertyHandleValuePair> TPropertyHandleValuePairList;

class IInstancePropertyCore
{
public:
    virtual ~IInstancePropertyCore() {}

    /**
     *	Find a given instance property by name. May return an invalid handle.
     */
    virtual CUICDMPropertyHandle
    GetAggregateInstancePropertyByName(CUICDMInstanceHandle inInstance,
                                       const TCharStr &inStr) const = 0;
    /**
     *	Get the entire list of instance properties;
     */
    virtual void GetAggregateInstanceProperties(CUICDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const = 0;
    // Get the properties that are stored only on this instance, not on any parents.
    virtual void GetSpecificInstancePropertyValues(CUICDMInstanceHandle inHandle,
                                                   TPropertyHandleValuePairList &outValues) = 0;
    /**
     *	Return true if this property is on this instance or one of its parents.
     */
    virtual bool HasAggregateInstanceProperty(CUICDMInstanceHandle inInstance,
                                              CUICDMPropertyHandle inProperty) const = 0;

    /**
     *	Does this value match the approperty property type.
     *	error - PropertyNotFound
     *	error - ValueTypeError
     */
    virtual void CheckValue(CUICDMInstanceHandle inInstance, CUICDMPropertyHandle inProperty,
                            const SValue &inValue) const = 0;
    /**
     *	Get this property from this instance.   This includes looking up the default value.  If the
     *instance doesn't have the property
     *	and it has no default the return value is false.
     *	error - InstanceNotFound
     *	error - PropertyNotFound if this property doesn't exist on this instance or doesn't exist.
     */
    virtual bool GetInstancePropertyValue(CUICDMInstanceHandle inHandle,
                                          CUICDMPropertyHandle inProperty,
                                          SValue &outValue) const = 0;
    /**
     *	Set this property on this instance.
     *	error - InstanceNotFound
     *	error - PropertyNotFound if this property doesn't exist on this instance or doesn't exist.
     *	error - ValueTypeError if this value is of the wrong type
     */
    virtual void SetInstancePropertyValue(CUICDMInstanceHandle inHandle,
                                          CUICDMPropertyHandle inProperty,
                                          const SValue &inValue) = 0;
};

typedef std::shared_ptr<IInstancePropertyCore> TInstancePropertyCorePtr;

class IStringTable;

/**
 *	Primary interface to data model system.  This allows you to define 'classes', which are
 *really just
 *	collections of properties, and instances of those classes.  A zero handle value is regarded
 *as empty.
 */
class IDataCore : public IHandleBase, public IInstancePropertyCore
{
public:
    virtual ~IDataCore() {}

    // Bookkeeping
    virtual IStringTable &GetStringTable() const = 0;
    virtual std::shared_ptr<IStringTable> GetStringTablePtr() const = 0;

    //===============================================================
    //	Instances
    //===============================================================
    /**
     *	Create a new instance of a class.  Instances can be named, in which case
     *	they are sometimes treated differently when serialized.  If the name is null
     *	or has no length then it is ignored.  If the name is non-null, then a check is
     *	run as named instances must have unique names.  When a named instance is a parent
     *	of another instance, only its name is serialized out; not its handle thus you can safely
     *	use names as inter-application instance handles.
     *	May use a target id to specify what the ideal id is; id must not exist.
     *	- error DuplicateInstanceName
     */
    virtual CUICDMInstanceHandle
    CreateInstance(CUICDMInstanceHandle inTargetId = CUICDMInstanceHandle()) = 0;
    /**
     *	Delete an instance.
     *	error - InstanceNotFound
     */
    virtual void DeleteInstance(CUICDMInstanceHandle inHandle) = 0;
    /**
     *	Return all the instances in the system.
     */
    virtual void GetInstances(TInstanceHandleList &outInstances) const = 0;
    /**
     *	Return all the instances in the system that is derived from parent.
     */
    virtual void GetInstancesDerivedFrom(TInstanceHandleList &outInstances,
                                         CUICDMInstanceHandle inParentHandle) const = 0;

    //===============================================================
    //	Instance Derivation
    //===============================================================
    /**
     *	Derive an instance from another instance.  This implies a rebuild of
     *	the aggregate properties of a given instance.
     *	error - InstanceNotFound if either instance or parent aren't found
     */
    virtual void DeriveInstance(CUICDMInstanceHandle inInstance, CUICDMInstanceHandle inParent) = 0;
    /**
     *	Return a count of the parent's of a given instance.
     *	error - InstanceNotFound
     */
    virtual void GetInstanceParents(CUICDMInstanceHandle inHandle,
                                    TInstanceHandleList &outParents) const = 0;

    /**
     *	Returns true if the instance is derived from the parent somehow.
     */
    virtual bool IsInstanceOrDerivedFrom(CUICDMInstanceHandle inInstance,
                                         CUICDMInstanceHandle inParent) const = 0;
    //===============================================================

    //===============================================================
    //	Class Properties
    //===============================================================
    /**
     *	Add a new property to a given instances.  You can use this to overshadow properties
     *	on base instances but you can't add two properties of the same name to a given
     *	instance.
     *	error - InstanceNotFound
     *	error - PropertyExists if the property already exists on this class
     *	error - ValueTypeError if the default value exists and is if a different type than
     *inPropType
     */
    virtual CUICDMPropertyHandle AddProperty(CUICDMInstanceHandle inInstance, TCharPtr inName,
                                             DataModelDataType::Value inPropType) = 0;
    /**
     *	Return the property definition that corresponds to a given property value.
     */
    virtual const SUICDMPropertyDefinition &GetProperty(CUICDMPropertyHandle inProperty) const = 0;
    /**
     *	Return the properties specific to this instance, not including properties
     *	gained by derivation
     */
    virtual void GetInstanceProperties(CUICDMInstanceHandle inInstance,
                                       TPropertyHandleList &outProperties) const = 0;
    /**
     *	Remove a property from an instance.  Instances will no long have this property accessible
     *nor will
     *	child instances.
     */
    virtual void RemoveProperty(CUICDMPropertyHandle inProperty) = 0;
    /**
     *	Copy properties (definitions & values) specific to one instance, not including properties
     *	gained by derivation, to another instance.
     */
    virtual void CopyInstanceProperties(CUICDMInstanceHandle inSrcInstance,
                                        CUICDMInstanceHandle inDestInstance) = 0;
    /**
     *	Removed cached intermediate values from the instance.  UICDM pull properties from the
     *inheritance hierarchy chain
     *  up to the current instance when someone requests a property in order to make the next
     *request quicker.  This breaks
     *	some forms of updating where a parent's default value gets changed and the instance won't
     *reflect it (until you
     *	save/load the file).
     */
    //===============================================================
    virtual void RemoveCachedValues(CUICDMInstanceHandle inInstance) = 0;

    //===============================================================
    //	Handle validation
    //===============================================================
    /**
     *	Is valid and is instances
     */
    virtual bool IsInstance(int inHandle) const = 0;
    /**
     * is valid and is property
     */
    virtual bool IsProperty(int inHandle) const = 0;
};

typedef std::shared_ptr<IDataCore> TDataCorePtr;

class IPropertySystem : public IHandleBase
{
public:
    virtual ~IPropertySystem() {}

    virtual CUICDMInstanceHandle CreateInstance() = 0;
    virtual void DeleteInstance(CUICDMInstanceHandle inHandle) = 0;
    virtual void DeriveInstance(CUICDMInstanceHandle inInstance, CUICDMInstanceHandle inParent) = 0;

    virtual void GetInstances(TInstanceHandleList &outInstances) const = 0;

    virtual CUICDMPropertyHandle AddProperty(CUICDMInstanceHandle inInstance, TCharPtr inName,
                                             DataModelDataType::Value inPropType) = 0;

    virtual CUICDMPropertyHandle
    GetAggregateInstancePropertyByName(CUICDMInstanceHandle inInstance,
                                       const TCharStr &inStr) const = 0;
    virtual void GetAggregateInstanceProperties(CUICDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const = 0;
    virtual bool HasAggregateInstanceProperty(CUICDMInstanceHandle inInstance,
                                              CUICDMPropertyHandle inProperty) const = 0;

    virtual bool GetInstancePropertyValue(CUICDMInstanceHandle inHandle,
                                          CUICDMPropertyHandle inProperty,
                                          SValue &outValue) const = 0;
    virtual void SetInstancePropertyValue(CUICDMInstanceHandle inHandle,
                                          CUICDMPropertyHandle inProperty,
                                          const SValue &inValue) = 0;

    virtual bool IsInstanceOrDerivedFrom(CUICDMInstanceHandle inInstance,
                                         CUICDMInstanceHandle inParent) const = 0;

    virtual DataModelDataType::Value GetDataType(CUICDMPropertyHandle inProperty) const = 0;
    virtual TCharStr GetName(CUICDMPropertyHandle inProperty) const = 0;
    virtual TCharStr GetFormalName(CUICDMInstanceHandle inInstance,
                                   CUICDMPropertyHandle inProperty) const = 0;
    virtual AdditionalMetaDataType::Value
    GetAdditionalMetaDataType(CUICDMInstanceHandle inInstance,
                              CUICDMPropertyHandle inProperty) const = 0;
    virtual TMetaDataData GetAdditionalMetaDataData(CUICDMInstanceHandle inInstance,
                                                    CUICDMPropertyHandle inProperty) const = 0;
    virtual CUICDMInstanceHandle GetPropertyOwner(CUICDMPropertyHandle inProperty) const = 0;
};
typedef std::shared_ptr<IPropertySystem> TPropertySystemPtr;

template <typename TCoreType>
inline SValue GetInstancePropertyValue(TCoreType inCore, CUICDMInstanceHandle inInstance,
                                       CUICDMPropertyHandle inProperty)
{
    SValue retval;
    if (!inCore->GetInstancePropertyValue(inInstance, inProperty, retval))
        throw PropertyNotFound(L"");
    return retval;
}

template <typename TDataType, typename TCoreType>
inline TDataType GetSpecificInstancePropertyValue(TCoreType inDataCore,
                                                  CUICDMInstanceHandle inInstance,
                                                  CUICDMPropertyHandle inProperty)
{
    return UICDM::get<TDataType>(GetInstancePropertyValue(inDataCore, inInstance, inProperty));
}

template <typename TDataType, typename TCoreType>
inline TDataType GetNamedInstancePropertyValue(TCoreType inDataCore,
                                               CUICDMInstanceHandle inInstance,
                                               const TCharStr &inName)
{
    CUICDMPropertyHandle theProperty =
        inDataCore->GetAggregateInstancePropertyByName(inInstance, inName);
    return GetSpecificInstancePropertyValue<TDataType, TCoreType>(inDataCore, inInstance,
                                                                  theProperty);
}

inline CUICDMPropertyHandle AddPropertyWithValue(IPropertySystem &inPropertySystem,
                                                 CUICDMInstanceHandle inInstance, TCharPtr inName,
                                                 DataModelDataType::Value inDataType,
                                                 const SValue &inValue)
{
    CUICDMPropertyHandle theProperty(inPropertySystem.AddProperty(inInstance, inName, inDataType));
    inPropertySystem.SetInstancePropertyValue(inInstance, theProperty, inValue);
    return theProperty;
}
}

#endif
