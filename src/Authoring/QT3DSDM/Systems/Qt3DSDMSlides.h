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
#ifndef UICDMSLIDESH
#define UICDMSLIDESH
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSlideCore.h"

namespace qt3dsdm {
/**
 *	Some properties point to instances which act like extended properties.
 *	An example of this would be the images on materials.  If a material has an
 *	image in one of its slots, that image should be duplicated upon property unlink
 *	and deleted upon property link.  It (and its animations) should be duplicated upon
 *	create slide and deleted upon slide destruction such that the concept that the image
 *	is really a property on the material stays sound.
 */
class IPropertyInstanceInfo
{
public:
    virtual ~IPropertyInstanceInfo() {}

    /**
     *	Return the instance that relates to this property
     */
    virtual Qt3DSDMInstanceHandle GetInstanceForProperty(const SValue &inValue) = 0;

    /**
     *	Duplicate this instance and whichever properties and animations you desire,
     *	returning a new data model value that will be set on the newly created property.
     *	Don't forget to associate the instance with a the dest slide.  I doubt things
     *	will work correctly if you do not.
     */
    virtual SValue CreateInstanceForProperty(CUICDMSlideHandle inSourceSlide,
                                             CUICDMSlideHandle inDestSlide,
                                             Qt3DSDMInstanceHandle inInstance) = 0;
};

typedef std::shared_ptr<IPropertyInstanceInfo> TPropertyInstanceInfoPtr;

struct SInstanceSlideInformation
{
    // The slide associated with the instance
    CUICDMSlideHandle m_AssociatedSlide;
    // The master slide of the component.
    CUICDMSlideHandle m_MasterSlide;
    // The active slide of the component.
    CUICDMSlideHandle m_ActiveSlide;
    // The current milliseconds of the active slide on the component.
    long m_ComponentMilliseconds;

    SInstanceSlideInformation()
        : m_ComponentMilliseconds(0)
    {
    }
    SInstanceSlideInformation(CUICDMSlideHandle inAssoc, CUICDMSlideHandle inMaster,
                              CUICDMSlideHandle inActive, long inMilliseconds)
        : m_AssociatedSlide(inAssoc)
        , m_MasterSlide(inMaster)
        , m_ActiveSlide(inActive)
        , m_ComponentMilliseconds(inMilliseconds)
    {
    }
};

class ISlideSystem
{
public:
    virtual ~ISlideSystem() {}
    virtual CUICDMSlideHandle CreateMasterSlide() = 0;
    virtual CUICDMSlideHandle CreateSlide(CUICDMSlideHandle inMaster, int inIndex = -1) = 0;
    // Duplicate this slide and put it into the master's child lists at index.
    virtual CUICDMSlideHandle DuplicateSlide(CUICDMSlideHandle inSourceSlide, int inDestIndex) = 0;
    virtual CUICDMSlideHandle GetMasterSlide(CUICDMSlideHandle inSlide) const = 0;
    virtual bool IsMasterSlide(CUICDMSlideHandle inSlide) const = 0;
    virtual CUICDMSlideHandle GetMasterSlideByComponentGuid(SLong4 inGuid) const = 0;
    // Indexes are 1 based.  Index 0 refers to the master slide; you can't delete this.
    virtual void DeleteSlideByIndex(CUICDMSlideHandle inMaster, size_t inIndex) = 0;
    virtual void GetSlideReferencedInstances(CUICDMSlideHandle inMaster, size_t inIndex,
                                             TInstanceHandleList &outReferencedInstances) = 0;
    virtual CUICDMSlideHandle GetSlideByIndex(CUICDMSlideHandle inMaster, size_t inIndex) const = 0;
    virtual void SetActiveSlide(CUICDMSlideHandle inMaster, size_t inIndex) = 0;
    virtual void RearrangeSlide(CUICDMSlideHandle inMaster, size_t inOldIndex,
                                size_t inNewIndex) = 0;
    /**
     *	Set the current component time.  This object will figure out the component from the slide.
     */
    virtual void SetComponentSeconds(CUICDMSlideHandle inSlide, float inSeconds) = 0;
    /**
     *	Return the current time of the component
     */
    virtual float GetComponentSeconds(CUICDMSlideHandle inSlide) const = 0;
    virtual long GetComponentSecondsLong(CUICDMSlideHandle inSlide) const = 0;
    virtual long GetComponentSecondsLong(Qt3DSDMInstanceHandle inInstance) const = 0;

    // The fastest way possible, get all of the slide information for this instance.
    virtual SInstanceSlideInformation
    GetInstanceSlideInformation(Qt3DSDMInstanceHandle inInstance) const = 0;
    /**
     *	Slide count includes the master slide;
     */
    virtual size_t GetSlideCount(CUICDMSlideHandle inMaster) const = 0;
    /**
     * Use the instance for storing information such as name, or the GUID of the object
     * this slide links to.
     */
    virtual Qt3DSDMInstanceHandle GetSlideInstance(CUICDMSlideHandle inSlide) const = 0;
    /**
     *	Reverse lookup into the slide system so you can match slides to instances.
     */
    virtual CUICDMSlideHandle GetSlideByInstance(Qt3DSDMInstanceHandle inInstance) const = 0;
    /**
     *	Slide may be either a master slide
     */
    virtual void AssociateInstanceWithSlide(CUICDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance) = 0;
    virtual CUICDMSlideHandle GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance) const = 0;
    virtual void GetAssociatedInstances(
        CUICDMSlideHandle inMaster,
        std::vector<std::pair<CUICDMSlideHandle, Qt3DSDMInstanceHandle>> &outAssociations) const = 0;
    virtual void GetAssociatedInstances(CUICDMSlideHandle inSlide,
                                        TInstanceHandleList &outAssociations) const = 0;
    virtual void LinkProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void UnlinkProperty(Qt3DSDMInstanceHandle inInstance,
                                Qt3DSDMPropertyHandle inProperty) = 0;
    virtual bool IsPropertyLinked(Qt3DSDMInstanceHandle inInstance,
                                  Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual bool CanPropertyBeLinked(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual void GetUnionOfProperties(CUICDMSlideHandle inSlide1, CUICDMSlideHandle inSlide,
                                      TInstancePropertyPairList &outProperties) const = 0;

    virtual bool GetSlidePropertyValue(size_t inSlide, Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, SValue &outValue) = 0;

    virtual void SetActiveSlide(CUICDMSlideHandle inSlide) = 0;
    virtual CUICDMSlideHandle GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty) const = 0;

    virtual bool SlideValid(CUICDMSlideHandle inSlide) const = 0;
    virtual int GetSlideIndex(CUICDMSlideHandle inSlide) const = 0;
    virtual int GetActiveSlideIndex(CUICDMSlideHandle inMaster) const = 0;
    virtual CUICDMSlideHandle GetActiveSlide(CUICDMSlideHandle inMaster) const = 0;
    virtual Qt3DSDMInstanceHandle GetSlideSelectedInstance(CUICDMSlideHandle inSlide) const = 0;
    virtual void SetSlideSelectedInstance(CUICDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance) = 0;

    virtual CUICDMSlideHandle GetApplicableSlide(Qt3DSDMInstanceHandle inHandle,
                                                 Qt3DSDMPropertyHandle inProperty) = 0;

    virtual bool GetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty,
                                          SValue &outValue) const = 0;
    virtual bool GetCanonicalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMPropertyHandle inProperty,
                                                   SValue &outValue) const = 0;
    virtual void ForceSetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                               Qt3DSDMInstanceHandle inInstance,
                                               Qt3DSDMPropertyHandle inProperty,
                                               const SValue &inValue) = 0;

    /**
     *	Let this object know that this property will sometimes reference another property
     *	and give an outside entity the chance to create new objects and properties when
     *	the property is unlinked and linked.
     */
    virtual void RegisterPropertyInstance(Qt3DSDMPropertyHandle inPropertyHandle,
                                          TPropertyInstanceInfoPtr inPropertyInfo) = 0;
};

typedef std::shared_ptr<ISlideSystem> TSlideSystemPtr;
}

#endif