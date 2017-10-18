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
#ifndef UICDMSIGNALSH
#define UICDMSIGNALSH
#include "UICDMDataTypes.h"
#include "UICDMHandles.h"
#include "UICDMAnimation.h"
#include "UICDMActionInfo.h"
#include "UICDMValue.h"
#include <functional>
#include <string>

namespace qt3dsdm {
// One may notice this looks a lot like std::signals.
// One may also notice that I chose to hide the implementation completely instead
// taking the route of boost/function.hpp instead.

// This is because boost/signal.hpp includes boost/any.hpp and boost/any.hpp
// doesn't currently compile in studio due to a #define problem.
// Strings.h defines placeholder to be a number and boost/any.hpp uses
// an inner class named placeholder...
class ISignalConnection
{
public:
    virtual ~ISignalConnection() {}
};

class ISignalItem
{
public:
    virtual ~ISignalItem() {}
};

typedef std::shared_ptr<ISignalItem> TSignalItemPtr;

typedef std::shared_ptr<ISignalConnection> TSignalConnectionPtr;

class IInstancePropertyCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
            &inCallback) = 0;
};

class IInstancePropertyCoreSignalSender : public ISignalItem
{
public:
    virtual void SignalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMPropertyHandle inProperty,
                                             const SValue &inValue) = 0;
};

class IDataCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeforeInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceDerived(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceParentRemoved(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectPropertyAdded(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                    TCharPtr, DataModelDataType::Value)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyRemoved(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr,
                                   DataModelDataType::Value)> &inCallback) = 0;
};

class IDataCoreSignalSender : public ISignalItem
{
public:
    virtual void SignalInstanceCreated(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SignalBeforeInstanceDeleted(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SignalInstanceDeleted(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SignalInstanceDerived(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMInstanceHandle inParent) = 0;
    virtual void SignalInstanceParentRemoved(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMInstanceHandle inParent) = 0;
    virtual void SignalPropertyAdded(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                     DataModelDataType::Value inDataType) = 0;
    virtual void SignalPropertyRemoved(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                       DataModelDataType::Value inDataType) = 0;
};

class ISlideCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeforeSlideDeleted(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideDerived(
        const std::function<void(CUICDMSlideHandle, CUICDMSlideHandle, int)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstancePropertyValueSet(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstancePropertyValueRemoved(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectSlideTimeChanged(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
};

class ISlideCoreSignalSender : public ISignalItem
{
public:
    virtual void SendSlideCreated(CUICDMSlideHandle inSlide) = 0;
    virtual void SendBeforeSlideDeleted(CUICDMSlideHandle inSlide) = 0;
    virtual void SendSlideDeleted(CUICDMSlideHandle inSlide) = 0;
    virtual void SendSlideDerived(CUICDMSlideHandle inSlide, CUICDMSlideHandle inParent,
                                  int inIndex) = 0;
    virtual void SendPropertyValueSet(CUICDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inParent, const SValue &inValue) = 0;
    // This gives clients a chance to override a property value the first time it is set on a slide
    virtual void SendPropertyValueRemoved(CUICDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inParent, const SValue &inValue) = 0;
    virtual void SendSlideTimeChanged(CUICDMSlideHandle inSlide) = 0;
};

class ISlideGraphCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectGraphCreated(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectGraphDeleted(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectGraphActiveSlide(
        const std::function<void(CUICDMSlideGraphHandle, CUICDMSlideHandle)> &inCallback) = 0;
};

class ISlideGraphCoreSignalSender : public ISignalItem
{
public:
    virtual void SendGraphCreated(CUICDMSlideGraphHandle inGraph, CUICDMSlideHandle inSlide) = 0;
    virtual void SendGraphDeleted(CUICDMSlideGraphHandle inGraph, CUICDMSlideHandle inSlide) = 0;
    virtual void SendInstanceAssociated(CUICDMSlideGraphHandle inGraph, CUICDMSlideHandle inSlide,
                                        Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendInstanceDissociated(CUICDMSlideGraphHandle inGraph, CUICDMSlideHandle inSlide,
                                         Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendGraphActiveSlide(CUICDMSlideGraphHandle inGraph,
                                      CUICDMSlideHandle inSlide) = 0;
};

class IAnimationCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectAnimationCreated(
        const std::function<void(CUICDMAnimationHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectBeforeAnimationDeleted(
        const std::function<void(CUICDMAnimationHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectAnimationDeleted(
        const std::function<void(CUICDMAnimationHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(CUICDMAnimationHandle, CUICDMKeyframeHandle, const TKeyframe &)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeforeKeyframeErased(const std::function<void(CUICDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(CUICDMAnimationHandle, CUICDMKeyframeHandle, const TKeyframe &)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectBeforeAllKeyframesErased(
        const std::function<void(CUICDMAnimationHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeUpdated(
        const std::function<void(CUICDMKeyframeHandle, const TKeyframe &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(CUICDMAnimationHandle, bool)> &inCallback) = 0;
};

class IAnimationCoreSignalSender : public ISignalItem
{
public:
    virtual void SendAnimationCreated(CUICDMAnimationHandle inAnimation, CUICDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) = 0;
    virtual void SendBeforeAnimationDeleted(CUICDMAnimationHandle inAnimation) = 0;
    virtual void SendAnimationDeleted(CUICDMAnimationHandle inAnimation, CUICDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) = 0;
    virtual void SendKeyframeInserted(CUICDMAnimationHandle inAnimation,
                                      CUICDMKeyframeHandle inKeyframe, const TKeyframe &inData) = 0;
    virtual void SendBeforeKeyframeErased(CUICDMKeyframeHandle inAnimation) = 0;
    virtual void SendKeyframeErased(CUICDMAnimationHandle inAnimation,
                                    CUICDMKeyframeHandle inKeyframe, const TKeyframe &inData) = 0;
    virtual void SendBeforeAllKeyframesErased(CUICDMAnimationHandle inAnimation) = 0;
    virtual void SendKeyframeUpdated(CUICDMKeyframeHandle inKeyframe, const TKeyframe &inData) = 0;
    virtual void SendFirstKeyframeDynamicSet(CUICDMAnimationHandle inAnimation,
                                             bool inKeyframeDynamic) = 0;
};

class IActionCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectTriggerObjectSet(
        const std::function<void(CUICDMActionHandle, SObjectRefType &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectTargetObjectSet(
        const std::function<void(CUICDMActionHandle, SObjectRefType &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectEventSet(
        const std::function<void(CUICDMActionHandle, const wstring &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerSet(
        const std::function<void(CUICDMActionHandle, const wstring &)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectHandlerArgumentAdded(
        const std::function<void(CUICDMActionHandle, CUICDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerArgumentRemoved(
        const std::function<void(CUICDMActionHandle, CUICDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(CUICDMHandlerArgHandle, const SValue &)> &inCallback) = 0;
};

class IActionCoreSignalSender : public ISignalItem
{
public:
    virtual void SendTriggerObjectSet(CUICDMActionHandle inAction,
                                      SObjectRefType &inTriggerObject) = 0;
    virtual void SendTargetObjectSet(CUICDMActionHandle inAction,
                                     SObjectRefType &inTargetObject) = 0;
    virtual void SendEventSet(CUICDMActionHandle inAction, const wstring &inEventHandle) = 0;
    virtual void SendHandlerSet(CUICDMActionHandle inAction, const wstring &inActionName) = 0;

    virtual void SendHandlerArgumentAdded(CUICDMActionHandle inAction,
                                          CUICDMHandlerArgHandle inHandlerArgument,
                                          const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                          DataModelDataType::Value inValueType) = 0;
    virtual void SendHandlerArgumentRemoved(CUICDMActionHandle inAction,
                                            CUICDMHandlerArgHandle inHandlerArgument,
                                            const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                            DataModelDataType::Value inValueType) = 0;
    virtual void SendHandlerArgumentValueSet(CUICDMHandlerArgHandle inHandlerArgument,
                                             const SValue &inValue) = 0;
};

class IActionSystemSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(CUICDMActionHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(CUICDMActionHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
};

class IActionSystemSignalSender : public ISignalItem
{
public:
    virtual void SendActionCreated(CUICDMActionHandle inAction, CUICDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendActionDeleted(CUICDMActionHandle inAction, CUICDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
};

class ICustomPropCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyModified(
        const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(CUICDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(CUICDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(CUICDMEventHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(CUICDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(CUICDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(CUICDMHandlerHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(CUICDMHandlerParamHandle, CUICDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(CUICDMHandlerParamHandle, CUICDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(CUICDMHandlerParamHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) = 0;
};

class ICustomPropCoreSignalSender : public ISignalItem
{
public:
    virtual void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) = 0;

    virtual void SendCustomEventCreated(CUICDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventDeleted(CUICDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventModified(CUICDMEventHandle inEvent) = 0;

    virtual void SendCustomHandlerCreated(CUICDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerDeleted(CUICDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerModified(CUICDMHandlerHandle inHandler) = 0;

    virtual void SendCustomHandlerParamCreated(CUICDMHandlerParamHandle inParameter,
                                               CUICDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamDeleted(CUICDMHandlerParamHandle inParameter,
                                               CUICDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamModified(CUICDMHandlerParamHandle inParameter) = 0;

    virtual void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) = 0;
};

class ISlideSystemSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectMasterCreated(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectMasterDeleted(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideCreated(
        const std::function<void(CUICDMSlideHandle, int, CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideDeleted(
        const std::function<void(CUICDMSlideHandle, int, CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideRearranged(
        const std::function<void(CUICDMSlideHandle, int, int)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(CUICDMSlideHandle, int, CUICDMSlideHandle, CUICDMSlideHandle)>
            &inCallback) = 0;
};

class ISlideSystemSignalSender : public ISignalItem
{
public:
    virtual void SendMasterCreated(CUICDMSlideHandle inMaster) = 0;
    virtual void SendMasterDeleted(CUICDMSlideHandle inMaster) = 0;
    virtual void SendSlideCreated(CUICDMSlideHandle inMaster, int inIndex,
                                  CUICDMSlideHandle inSlide) = 0;
    virtual void SendSlideDeleted(CUICDMSlideHandle inMaster, int inIndex,
                                  CUICDMSlideHandle inSlide) = 0;
    virtual void SendSlideRearranged(CUICDMSlideHandle inMaster, int inOldIndex,
                                     int inNewIndex) = 0;
    virtual void SendInstanceAssociated(CUICDMSlideHandle inMaster,
                                        Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendInstanceDissociated(CUICDMSlideHandle inMaster,
                                         Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendPropertyLinked(CUICDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendPropertyUnlinked(CUICDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendActiveSlide(CUICDMSlideHandle inMaster, int inIndex,
                                 CUICDMSlideHandle inOldSlide, CUICDMSlideHandle inNewSlide) = 0;
};

class IStudioFullSystemSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideRearranged(
        const std::function<void(CUICDMSlideHandle, int, int)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeginComponentSeconds(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectComponentSeconds(const std::function<void(CUICDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(CUICDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(CUICDMSlideHandle, int, CUICDMSlideHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr
    ConnectAnimationCreated(const std::function<void(CUICDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectAnimationDeleted(const std::function<void(CUICDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(CUICDMAnimationHandle, CUICDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(CUICDMAnimationHandle, CUICDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectKeyframeUpdated(const std::function<void(CUICDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(CUICDMAnimationHandle, bool)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(CUICDMActionHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(CUICDMActionHandle, CUICDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectTriggerObjectSet(const std::function<void(CUICDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectTargetObjectSet(const std::function<void(CUICDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(CUICDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(CUICDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(CUICDMHandlerArgHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyModified(
        const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(CUICDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(CUICDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(CUICDMEventHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(CUICDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(CUICDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(CUICDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(CUICDMHandlerParamHandle, CUICDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(CUICDMHandlerParamHandle, CUICDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(CUICDMHandlerParamHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) = 0;
};

class IStudioFullSystemSignalSender : public ISignalItem
{
public:
    virtual void SendSlideCreated(CUICDMSlideHandle inSlide) = 0;
    virtual void SendSlideDeleted(CUICDMSlideHandle inSlide) = 0;
    virtual void SendSlideRearranged(CUICDMSlideHandle inMaster, int inOldIndex,
                                     int inNewIndex) = 0;
    virtual void SendComponentSeconds(CUICDMSlideHandle inMaster) = 0;
    virtual void SendBeginComponentSeconds(CUICDMSlideHandle inMaster) = 0;
    virtual void SendPropertyLinked(CUICDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendPropertyUnlinked(CUICDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendActiveSlide(CUICDMSlideHandle inMaster, int inIndex,
                                 CUICDMSlideHandle inSlide) = 0;

    virtual void SendInstanceCreated(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendInstanceDeleted(Qt3DSDMInstanceHandle inInstance) = 0;

    virtual void SendAnimationCreated(CUICDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendAnimationDeleted(CUICDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendKeyframeInserted(CUICDMAnimationHandle inAnimation,
                                      CUICDMKeyframeHandle inKeyframe) = 0;
    virtual void SendKeyframeErased(CUICDMAnimationHandle inAnimation,
                                    CUICDMKeyframeHandle inKeyframe) = 0;
    virtual void SendKeyframeUpdated(CUICDMKeyframeHandle inKeyframe) = 0;
    virtual void SendConnectFirstKeyframeDynamicSet(CUICDMAnimationHandle inAnimation,
                                                    bool inDynamic) = 0;

    virtual void SendInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                           Qt3DSDMPropertyHandle inProperty) = 0;

    virtual void SendActionCreated(CUICDMActionHandle inAction, CUICDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendActionDeleted(CUICDMActionHandle inAction, CUICDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendTriggerObjectSet(CUICDMActionHandle inAction) = 0;
    virtual void SendTargetObjectSet(CUICDMActionHandle inAction) = 0;
    virtual void SendEventSet(CUICDMActionHandle inAction) = 0;
    virtual void SendHandlerSet(CUICDMActionHandle inAction) = 0;
    virtual void SendHandlerArgumentValueSet(CUICDMHandlerArgHandle inHandlerArgument) = 0;

    virtual void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) = 0;
    virtual void SendCustomEventCreated(CUICDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventDeleted(CUICDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventModified(CUICDMEventHandle inEvent) = 0;
    virtual void SendCustomHandlerCreated(CUICDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerDeleted(CUICDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerModified(CUICDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamCreated(CUICDMHandlerParamHandle inParameter,
                                               CUICDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamDeleted(CUICDMHandlerParamHandle inParameter,
                                               CUICDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamModified(CUICDMHandlerParamHandle inParameter) = 0;
    virtual void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) = 0;
};

// Use this if you want to register for only a specific instance or specific property
template <typename TTransaction>
inline void MaybackCallbackInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 Qt3DSDMInstanceHandle inDesiredInstance,
                                                 Qt3DSDMPropertyHandle inDesiredProperty,
                                                 TTransaction inCallback)
{
    if ((!inDesiredInstance.Valid() || (inDesiredInstance == inInstance))
        && (!inDesiredProperty.Valid() || (inDesiredProperty == inProperty)))
        inCallback(inInstance, inProperty);
}

void SetUICDMSignalsEnabled(bool inEnabled);
// Defaults to true
bool AreUICDMSignalsEnabled();
}

#endif
