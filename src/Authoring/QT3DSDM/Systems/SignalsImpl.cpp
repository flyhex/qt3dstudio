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
#include "Qt3DSDMPrefix.h"
#ifdef WIN32
#pragma warning(disable : 4103 4512 4503)
#endif
#include "SignalsImpl.h"
#include <boost/signal.hpp>
#include <boost/signals/connection.hpp>

using namespace boost;

using namespace std;
using namespace boost::BOOST_SIGNALS_NAMESPACE;

namespace {
bool g_UICDMSignalsEnabled = true;

#define CHECK_SIGNALS_ENABLED()                                                                    \
    {                                                                                              \
        if (g_UICDMSignalsEnabled == false)                                                        \
            return;                                                                                \
    }
}

namespace qt3dsdm {
void SetUICDMSignalsEnabled(bool inEnabled)
{
    g_UICDMSignalsEnabled = inEnabled;
}
// Defaults to true
bool AreUICDMSignalsEnabled()
{
    return g_UICDMSignalsEnabled;
}
struct SBoostSignalConnection : public ISignalConnection
{
    Q_DISABLE_COPY(SBoostSignalConnection)

    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection m_connection;
    SBoostSignalConnection(const boost::BOOST_SIGNALS_NAMESPACE::connection &inConnection)
        : m_connection(inConnection)
    {
    }
};

#define CONNECT(x) TSignalConnectionPtr(new SBoostSignalConnection(x))
#define CONNECT_SIGNAL(x) CONNECT(x.connect(inCallback))

class CPropertyCoreSignaller : public IInstancePropertyCoreSignalProvider,
                               public IInstancePropertyCoreSignalSender
{
    boost::signal<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue)> m_PropertySignal;

public:
    TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
            &inCallback) override
    {
        return CONNECT(m_PropertySignal.connect(inCallback));
    }
    void SignalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override
    {
        CHECK_SIGNALS_ENABLED();
        m_PropertySignal(inInstance, inProperty, inValue);
    }
};

TSignalItemPtr CreatePropertyCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<IInstancePropertyCoreSignalProvider *>(new CPropertyCoreSignaller()));
}

class CDataCoreSignaller : public IDataCoreSignalProvider, public IDataCoreSignalSender
{
    boost::signal<void(Qt3DSDMInstanceHandle)> m_InstanceCreated;
    boost::signal<void(Qt3DSDMInstanceHandle)> m_BeforeInstanceDeleted;
    boost::signal<void(Qt3DSDMInstanceHandle)> m_InstanceDeleted;
    boost::signal<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> m_InstanceDerived;
    boost::signal<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> m_InstanceParentRemoved;
    boost::signal<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr, DataModelDataType::Value)>
        m_PropertyAdded;
    boost::signal<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr, DataModelDataType::Value)>
        m_PropertyRemoved;

public:
    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT(m_InstanceCreated.connect(inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectBeforeInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT(m_BeforeInstanceDeleted.connect(inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT(m_InstanceDeleted.connect(inCallback));
    }
    TSignalConnectionPtr ConnectInstanceDerived(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT(m_InstanceDerived.connect(inCallback));
    }
    TSignalConnectionPtr ConnectInstanceParentRemoved(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT(m_InstanceParentRemoved.connect(inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectPropertyAdded(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                    TCharPtr, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT(m_PropertyAdded.connect(inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectPropertyRemoved(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                      TCharPtr, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT(m_PropertyRemoved.connect(inCallback));
    }

    void SignalInstanceCreated(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceCreated(inInstance);
    }
    void SignalBeforeInstanceDeleted(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_BeforeInstanceDeleted(inInstance);
    }
    void SignalInstanceDeleted(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceDeleted(inInstance);
    }
    void SignalInstanceDerived(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMInstanceHandle inParent) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceDerived(inInstance, inParent);
    }
    void SignalInstanceParentRemoved(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMInstanceHandle inParent) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceParentRemoved(inInstance, inParent);
    }
    void SignalPropertyAdded(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                     DataModelDataType::Value inDataType) override
    {
        CHECK_SIGNALS_ENABLED();
        m_PropertyAdded(inInstance, inProperty, inName, inDataType);
    }
    void SignalPropertyRemoved(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                       DataModelDataType::Value inDataType) override
    {
        CHECK_SIGNALS_ENABLED();
        m_PropertyRemoved(inInstance, inProperty, inName, inDataType);
    }
};

TSignalItemPtr CreateDataCoreSignaller()
{
    return TSignalItemPtr(static_cast<IDataCoreSignalProvider *>(new CDataCoreSignaller()));
}

class CSlideCoreSignaller : public ISlideCoreSignalProvider, public ISlideCoreSignalSender
{
    boost::signal<void(Qt3DSDMSlideHandle)> m_SlideCreated;
    boost::signal<void(Qt3DSDMSlideHandle)> m_BeforeSlideDeleted;
    boost::signal<void(Qt3DSDMSlideHandle)> m_SlideDeleted;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, int)> m_SlideDerived;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
        m_InstancePropertyValueSet;
    boost::signal<SValue(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                  const SValue &)>
        m_InstancePropertyValueAdded;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
        m_InstancePropertyValueRemoved;
    boost::signal<void(Qt3DSDMSlideHandle)> m_SlideTimeChanged;

public:
    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideCreated);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_BeforeSlideDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideDeleted);
    }
    TSignalConnectionPtr ConnectSlideDerived(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, int)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideDerived);
    }
    TSignalConnectionPtr ConnectInstancePropertyValueSet(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstancePropertyValueSet);
    }
    virtual TSignalConnectionPtr ConnectInstancePropertyValueAdded(
        const std::function<SValue(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                     Qt3DSDMPropertyHandle, const SValue &)> &inCallback)
    {
        return CONNECT_SIGNAL(m_InstancePropertyValueAdded);
    }
    TSignalConnectionPtr ConnectInstancePropertyValueRemoved(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstancePropertyValueRemoved);
    }
    virtual TSignalConnectionPtr
    ConnectSlideTimeChanged(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideTimeChanged);
    }

    void SendSlideCreated(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideCreated(inSlide);
    }
    void SendBeforeSlideDeleted(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_BeforeSlideDeleted(inSlide);
    }
    void SendSlideDeleted(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideDeleted(inSlide);
    }
    void SendSlideDerived(Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent,
                                  int inIndex) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideDerived(inSlide, inParent, inIndex);
    }
    void SendPropertyValueSet(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstancePropertyValueSet(inSlide, inInstance, inProperty, inValue);
    }
    virtual SValue SendPropertyValueAdded(Qt3DSDMSlideHandle inSource, Qt3DSDMSlideHandle inDest,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, const SValue &inValue)
    {
        // Signals that return values tend to assert if there aren't
        // any clients for those signals.  Thus we only call them if
        // there are currently clients connected.
        // Good luck finding the number of connected items on a boost signal...
        size_t theNumSlots(m_InstancePropertyValueAdded.num_slots());
        if (theNumSlots)
            return m_InstancePropertyValueAdded(inSource, inDest, inInstance, inProperty, inValue);
        return inValue;
    }
    void SendPropertyValueRemoved(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override
    {
        m_InstancePropertyValueRemoved(inSlide, inInstance, inProperty, inValue);
    }
    void SendSlideTimeChanged(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideTimeChanged(inSlide);
    }
};

TSignalItemPtr CreateSlideCoreSignaller()
{
    return TSignalItemPtr(static_cast<ISlideCoreSignalProvider *>(new CSlideCoreSignaller()));
}

class CSlideGraphCoreSignaller : public ISlideGraphCoreSignalProvider,
                                 public ISlideGraphCoreSignalSender
{
    boost::signal<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> m_GraphCreated;
    boost::signal<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> m_GraphDeleted;
    boost::signal<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
        m_InstanceAssociated;
    boost::signal<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
        m_InstanceDissociated;
    boost::signal<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> m_GraphActiveSlide;
    boost::signal<void(Qt3DSDMSlideGraphHandle, float)> m_GraphSeconds;

public:
    TSignalConnectionPtr ConnectGraphCreated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_GraphCreated);
    }
    TSignalConnectionPtr ConnectGraphDeleted(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_GraphDeleted);
    }
    TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstanceAssociated);
    }
    TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstanceDissociated);
    }
    TSignalConnectionPtr ConnectGraphActiveSlide(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_GraphActiveSlide);
    }
    virtual TSignalConnectionPtr
    ConnectGraphSeconds(const std::function<void(Qt3DSDMSlideGraphHandle, float)> &inCallback)
    {
        return CONNECT_SIGNAL(m_GraphSeconds);
    }

    void SendGraphCreated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_GraphCreated(inGraph, inSlide);
    }
    void SendGraphDeleted(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_GraphDeleted(inGraph, inSlide);
    }
    void SendInstanceAssociated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide,
                                        Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceAssociated(inGraph, inSlide, inInstance);
    }
    void SendInstanceDissociated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide,
                                         Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceDissociated(inGraph, inSlide, inInstance);
    }
    void SendGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_GraphActiveSlide(inGraph, inSlide);
    }
    virtual void SendGraphSeconds(Qt3DSDMSlideGraphHandle inGraph, float inSeconds)
    {
        CHECK_SIGNALS_ENABLED();
        m_GraphSeconds(inGraph, inSeconds);
    }
};

TSignalItemPtr CreateSlideGraphCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<ISlideGraphCoreSignalProvider *>(new CSlideGraphCoreSignaller()));
}

class CAnimationCoreSignaller : public IAnimationCoreSignalProvider,
                                public IAnimationCoreSignalSender
{
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                Qt3DSDMPropertyHandle, size_t, EAnimationType)>
        m_AnimationCreated;
    boost::signal<void(Qt3DSDMAnimationHandle)> m_BeforeAnimationDeleted;
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                Qt3DSDMPropertyHandle, size_t, EAnimationType)>
        m_AnimationDeleted;
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)> m_KeyframeInserted;
    boost::signal<void(Qt3DSDMKeyframeHandle)> m_BeforeKeyframeErased;
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)> m_KeyframeErased;
    boost::signal<void(Qt3DSDMAnimationHandle)> m_BeforeAllKeyframesErased;
    boost::signal<void(Qt3DSDMKeyframeHandle, const TKeyframe &)> m_KeyframeUpdated;
    boost::signal<void(Qt3DSDMAnimationHandle, bool)> m_FirstKeyframeDynamic;

public:
    TSignalConnectionPtr ConnectAnimationCreated(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_AnimationCreated);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeAnimationDeleted(const std::function<void(Qt3DSDMAnimationHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_BeforeAnimationDeleted);
    }
    TSignalConnectionPtr ConnectAnimationDeleted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_AnimationDeleted);
    }
    TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_KeyframeInserted);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeKeyframeErased(const std::function<void(Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_BeforeKeyframeErased);
    }
    TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_KeyframeErased);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeAllKeyframesErased(const std::function<void(Qt3DSDMAnimationHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_BeforeAllKeyframesErased);
    }
    TSignalConnectionPtr ConnectKeyframeUpdated(
        const std::function<void(Qt3DSDMKeyframeHandle, const TKeyframe &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_KeyframeUpdated);
    }
    TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(Qt3DSDMAnimationHandle, bool)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_FirstKeyframeDynamic);
    }

    void SendAnimationCreated(Qt3DSDMAnimationHandle inAnimation, Qt3DSDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) override
    {
        CHECK_SIGNALS_ENABLED();
        m_AnimationCreated(inAnimation, inSlide, inInstance, inProperty, inIndex, inAnimationType);
    }
    void SendBeforeAnimationDeleted(Qt3DSDMAnimationHandle inAnimation) override
    {
        CHECK_SIGNALS_ENABLED();
        m_BeforeAnimationDeleted(inAnimation);
    }
    void SendAnimationDeleted(Qt3DSDMAnimationHandle inAnimation, Qt3DSDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) override
    {
        CHECK_SIGNALS_ENABLED();
        m_AnimationDeleted(inAnimation, inSlide, inInstance, inProperty, inIndex, inAnimationType);
    }
    void SendKeyframeInserted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) override
    {
        CHECK_SIGNALS_ENABLED();
        m_KeyframeInserted(inAnimation, inKeyframe, inData);
    }
    void SendBeforeKeyframeErased(Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        m_BeforeKeyframeErased(inKeyframe);
    }
    void SendKeyframeErased(Qt3DSDMAnimationHandle inAnimation,
                                    Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) override
    {
        CHECK_SIGNALS_ENABLED();
        m_KeyframeErased(inAnimation, inKeyframe, inData);
    }
    void SendBeforeAllKeyframesErased(Qt3DSDMAnimationHandle inAnimation) override
    {
        CHECK_SIGNALS_ENABLED();
        m_BeforeAllKeyframesErased(inAnimation);
    }
    void SendKeyframeUpdated(Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) override
    {
        CHECK_SIGNALS_ENABLED();
        m_KeyframeUpdated(inKeyframe, inData);
    }
    void SendFirstKeyframeDynamicSet(Qt3DSDMAnimationHandle inAnimation,
                                             bool inKeyframeDynamic) override
    {
        CHECK_SIGNALS_ENABLED();
        m_FirstKeyframeDynamic(inAnimation, inKeyframeDynamic);
    }
};

TSignalItemPtr CreateAnimationCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<IAnimationCoreSignalProvider *>(new CAnimationCoreSignaller()));
}

class CActionCoreSignaller : public IActionCoreSignalProvider, public IActionCoreSignalSender
{
    boost::signal<void(Qt3DSDMActionHandle, SObjectRefType &)> m_TriggerObjectSet;
    boost::signal<void(Qt3DSDMActionHandle, SObjectRefType &)> m_TargetObjectSet;
    boost::signal<void(Qt3DSDMActionHandle, const wstring &)> m_EventHandleSet;
    boost::signal<void(Qt3DSDMActionHandle, const wstring &)> m_HandlerHandleSet;

    boost::signal<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &, HandlerArgumentType::Value,
                DataModelDataType::Value)>
        m_HandlerArgumentAdded;
    boost::signal<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &, HandlerArgumentType::Value,
                DataModelDataType::Value)>
        m_HandlerArgumentRemoved;
    boost::signal<void(Qt3DSDMHandlerArgHandle, const SValue &)> m_HandlerArgumentValueSet;

public:
    TSignalConnectionPtr ConnectTriggerObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_TriggerObjectSet);
    }
    TSignalConnectionPtr ConnectTargetObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_TargetObjectSet);
    }
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_EventHandleSet);
    }
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_HandlerHandleSet);
    }

    TSignalConnectionPtr ConnectHandlerArgumentAdded(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_HandlerArgumentAdded);
    }
    TSignalConnectionPtr ConnectHandlerArgumentRemoved(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_HandlerArgumentRemoved);
    }
    TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(Qt3DSDMHandlerArgHandle, const SValue &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_HandlerArgumentValueSet);
    }

    void SendTriggerObjectSet(Qt3DSDMActionHandle inAction, SObjectRefType &inTriggerObject) override
    {
        CHECK_SIGNALS_ENABLED();
        m_TriggerObjectSet(inAction, inTriggerObject);
    }
    void SendTargetObjectSet(Qt3DSDMActionHandle inAction, SObjectRefType &inTargetObject) override
    {
        CHECK_SIGNALS_ENABLED();
        m_TargetObjectSet(inAction, inTargetObject);
    }
    void SendEventSet(Qt3DSDMActionHandle inAction, const wstring &inEventHandle) override
    {
        CHECK_SIGNALS_ENABLED();
        m_EventHandleSet(inAction, inEventHandle);
    }
    void SendHandlerSet(Qt3DSDMActionHandle inAction, const wstring &inHandlerHandle) override
    {
        CHECK_SIGNALS_ENABLED();
        m_HandlerHandleSet(inAction, inHandlerHandle);
    }

    void SendHandlerArgumentAdded(Qt3DSDMActionHandle inAction,
                                          Qt3DSDMHandlerArgHandle inHandlerArgument,
                                          const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                          DataModelDataType::Value inValueType) override
    {
        CHECK_SIGNALS_ENABLED();
        m_HandlerArgumentAdded(inAction, inHandlerArgument, inName, inArgType, inValueType);
    }
    void SendHandlerArgumentRemoved(Qt3DSDMActionHandle inAction,
                                            Qt3DSDMHandlerArgHandle inHandlerArgument,
                                            const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                            DataModelDataType::Value inValueType) override
    {
        CHECK_SIGNALS_ENABLED();
        m_HandlerArgumentRemoved(inAction, inHandlerArgument, inName, inArgType, inValueType);
    }
    void SendHandlerArgumentValueSet(Qt3DSDMHandlerArgHandle inHandlerArgument,
                                             const SValue &inValue) override
    {
        CHECK_SIGNALS_ENABLED();
        m_HandlerArgumentValueSet(inHandlerArgument, inValue);
    }
};

TSignalItemPtr CreateActionCoreSignaller()
{
    return TSignalItemPtr(static_cast<IActionCoreSignalProvider *>(new CActionCoreSignaller()));
}

class CActionSystemSignaller : public IActionSystemSignalProvider, public IActionSystemSignalSender
{
    boost::signal<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> m_ActionCreated;
    boost::signal<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> m_ActionDeleted;

public:
    TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_ActionCreated);
    }
    TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_ActionDeleted);
    }

    void SendActionCreated(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ActionCreated(inAction, inSlide, inOwner);
    }
    void SendActionDeleted(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ActionDeleted(inAction, inSlide, inOwner);
    }
};

TSignalItemPtr CreateActionSystemSignaller()
{
    return TSignalItemPtr(static_cast<IActionSystemSignalProvider *>(new CActionSystemSignaller()));
}

class CSlideSystemSignaller : public ISlideSystemSignalProvider, public ISlideSystemSignalSender
{
    boost::signal<void(Qt3DSDMSlideHandle)> m_MasterCreated;
    boost::signal<void(Qt3DSDMSlideHandle)> m_MasterDeleted;
    boost::signal<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> m_SlideCreated;
    boost::signal<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> m_SlideDeleted;
    boost::signal<void(Qt3DSDMSlideHandle, int, int)> m_SlideRearranged;
    boost::signal<void(Qt3DSDMSlideHandle, float)> m_ComponentSeconds;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> m_InstanceAssociated;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> m_InstanceDissociated;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> m_PropertyLinked;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> m_PropertyUnlinked;
    boost::signal<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle, Qt3DSDMSlideHandle)> m_ActiveSlide;

    boost::signal<SValue(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue)>
        m_ValueCreated;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue)>
        m_ValueDestroyed;

public:
    virtual TSignalConnectionPtr
    ConnectMasterCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_MasterCreated);
    }
    virtual TSignalConnectionPtr
    ConnectMasterDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_MasterDeleted);
    }
    TSignalConnectionPtr ConnectSlideCreated(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideCreated);
    }
    TSignalConnectionPtr ConnectSlideDeleted(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectSlideRearranged(const std::function<void(Qt3DSDMSlideHandle, int, int)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideRearranged);
    }
    virtual TSignalConnectionPtr
    ConnectComponentSeconds(const std::function<void(Qt3DSDMSlideHandle, float)> &inCallback)
    {
        return CONNECT_SIGNAL(m_ComponentSeconds);
    }
    TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstanceAssociated);
    }
    TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstanceDissociated);
    }
    TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_PropertyLinked);
    }
    TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_PropertyUnlinked);
    }
    TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle, Qt3DSDMSlideHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_ActiveSlide);
    }
    virtual TSignalConnectionPtr ConnectPropertyValueCreated(
        const std::function<SValue(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                     const SValue &)> &inCallback)
    {
        return CONNECT_SIGNAL(m_ValueCreated);
    }
    virtual TSignalConnectionPtr ConnectPropertyValueDestroyed(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback)
    {
        return CONNECT_SIGNAL(m_ValueDestroyed);
    }

    void SendMasterCreated(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        m_MasterCreated(inMaster);
    }
    void SendMasterDeleted(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        m_MasterDeleted(inMaster);
    }
    void SendSlideCreated(Qt3DSDMSlideHandle inMaster, int inIndex,
                                  Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideCreated(inMaster, inIndex, inSlide);
    }
    void SendSlideDeleted(Qt3DSDMSlideHandle inMaster, int inIndex,
                                  Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideDeleted(inMaster, inIndex, inSlide);
    }
    void SendSlideRearranged(Qt3DSDMSlideHandle inMaster, int inOldIndex, int inNewIndex) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideRearranged(inMaster, inOldIndex, inNewIndex);
    }
    virtual void SendComponentSeconds(Qt3DSDMSlideHandle inMaster, float inSeconds)
    {
        CHECK_SIGNALS_ENABLED();
        m_ComponentSeconds(inMaster, inSeconds);
    }
    void SendInstanceAssociated(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceAssociated(inMaster, inInstance);
    }
    void SendInstanceDissociated(Qt3DSDMSlideHandle inMaster,
                                         Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceDissociated(inMaster, inInstance);
    }
    void SendPropertyLinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        m_PropertyLinked(inMaster, inInstance, inProperty);
    }
    void SendPropertyUnlinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        m_PropertyUnlinked(inMaster, inInstance, inProperty);
    }
    void SendActiveSlide(Qt3DSDMSlideHandle inMaster, int inIndex,
                                 Qt3DSDMSlideHandle inOldSlide, Qt3DSDMSlideHandle inNewSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ActiveSlide(inMaster, inIndex, inOldSlide, inNewSlide);
    }

    virtual SValue SendPropertyValueCreated(Qt3DSDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance,
                                            Qt3DSDMPropertyHandle inProperty, const SValue &inValue)
    {
        size_t theNumSlots(m_ValueCreated.num_slots());
        if (theNumSlots)
            return m_ValueCreated(inSlide, inInstance, inProperty, inValue);
        return inValue;
    }
    virtual void SendPropertyValueDestroyed(Qt3DSDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance,
                                            Qt3DSDMPropertyHandle inProperty, const SValue &inValue)
    {
        CHECK_SIGNALS_ENABLED();
        m_ValueDestroyed(inSlide, inInstance, inProperty, inValue);
    }
};

TSignalItemPtr CreateSlideSystemSignaller()
{
    return TSignalItemPtr(static_cast<ISlideSystemSignalProvider *>(new CSlideSystemSignaller()));
}

class CCustomPropCoreSignaller : public ICustomPropCoreSignalProvider,
                                 public ICustomPropCoreSignalSender
{
    boost::signal<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> m_CustomPropertyCreated;
    boost::signal<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> m_CustomPropertyDeleted;
    boost::signal<void(Qt3DSDMPropertyHandle)> m_CustomPropertyModified;
    boost::signal<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> m_CustomEventCreated;
    boost::signal<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> m_CustomEventDeleted;
    boost::signal<void(Qt3DSDMEventHandle)> m_CustomEventModified;
    boost::signal<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> m_CustomHandlerCreated;
    boost::signal<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> m_CustomHandlerDeleted;
    boost::signal<void(Qt3DSDMHandlerHandle)> m_CustomHandlerModified;
    boost::signal<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> m_CustomHandlerParamCreated;
    boost::signal<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> m_CustomHandlerParamDeleted;
    boost::signal<void(Qt3DSDMHandlerParamHandle)> m_CustomHandlerParamModified;
    boost::signal<void(Qt3DSDMInstanceHandle, const TCharStr &)> m_CustomReferencesModified;

public:
    TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomPropertyCreated);
    }
    TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomPropertyDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomPropertyModified(const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomPropertyModified);
    }
    TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomEventCreated);
    }
    TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomEventDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(Qt3DSDMEventHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomEventModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerParamCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerParamDeleted);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(Qt3DSDMHandlerParamHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerParamModified);
    }
    TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomReferencesModified);
    }

    void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomPropertyCreated(inProp, inOwner);
    }
    void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomPropertyDeleted(inProp, inOwner);
    }
    void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomPropertyModified(inProp);
    }
    void SendCustomEventCreated(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomEventCreated(inEvent, inOwner);
    }
    void SendCustomEventDeleted(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomEventDeleted(inEvent, inOwner);
    }
    void SendCustomEventModified(Qt3DSDMEventHandle inEvent) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomEventModified(inEvent);
    }
    void SendCustomHandlerCreated(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerCreated(inHandler, inOwner);
    }
    void SendCustomHandlerDeleted(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerDeleted(inHandler, inOwner);
    }
    void SendCustomHandlerModified(Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerModified(inHandler);
    }
    void SendCustomHandlerParamCreated(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerParamCreated(inParameter, inHandler);
    }
    void SendCustomHandlerParamDeleted(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerParamDeleted(inParameter, inHandler);
    }
    void SendCustomHandlerParamModified(Qt3DSDMHandlerParamHandle inParameter) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerParamModified(inParameter);
    }
    void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomReferencesModified(inOwner, inString);
    }
};

TSignalItemPtr CreateCustomPropCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<ICustomPropCoreSignalProvider *>(new CCustomPropCoreSignaller()));
}

class CStudioFullSystemSignaller : public IStudioFullSystemSignalProvider,
                                   public IStudioFullSystemSignalSender
{
    boost::signal<void()> m_ChangeSetBegin;
    boost::signal<void()> m_ChangeSetEnd;
    boost::signal<void()> m_AnimationBegin;
    boost::signal<void()> m_AnimationEnd;
    boost::signal<void(Qt3DSDMSlideHandle)> m_SlideCreated;
    boost::signal<void(Qt3DSDMSlideHandle)> m_SlideDeleted;
    boost::signal<void(Qt3DSDMSlideHandle, int, int)> m_SlideRearranged;
    boost::signal<void(Qt3DSDMSlideHandle)> m_ComponentSeconds;
    boost::signal<void(Qt3DSDMSlideHandle)> m_BeginComponentSeconds;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> m_PropertyLinked;
    boost::signal<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> m_PropertyUnlinked;
    boost::signal<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> m_ActiveSlide;
    boost::signal<void(Qt3DSDMInstanceHandle)> m_InstanceCreated;
    boost::signal<void(Qt3DSDMInstanceHandle)> m_InstanceDeleted;
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
        m_AnimationCreated;
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
        m_AnimationDeleted;
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> m_KeyframeInserted;
    boost::signal<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> m_KeyframeErased;
    boost::signal<void(Qt3DSDMKeyframeHandle)> m_KeyframeUpdated;
    boost::signal<void(Qt3DSDMAnimationHandle, bool)> m_ConnectFirstKeyframeDynamicSet;
    boost::signal<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> m_InstancePropertyValue;
    boost::signal<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> m_ActionCreated;
    boost::signal<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> m_ActionDeleted;
    boost::signal<void(Qt3DSDMActionHandle)> m_TriggerObjectSet;
    boost::signal<void(Qt3DSDMActionHandle)> m_TargetObjectSet;
    boost::signal<void(Qt3DSDMActionHandle)> m_EventHandleSet;
    boost::signal<void(Qt3DSDMActionHandle)> m_HandlerHandleSet;
    boost::signal<void(Qt3DSDMHandlerArgHandle)> m_HandlerArgumentValueSet;
    boost::signal<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> m_CustomPropertyCreated;
    boost::signal<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> m_CustomPropertyDeleted;
    boost::signal<void(Qt3DSDMPropertyHandle)> m_CustomPropertyModified;
    boost::signal<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> m_CustomEventCreated;
    boost::signal<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> m_CustomEventDeleted;
    boost::signal<void(Qt3DSDMEventHandle)> m_CustomEventModified;
    boost::signal<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> m_CustomHandlerCreated;
    boost::signal<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> m_CustomHandlerDeleted;
    boost::signal<void(Qt3DSDMHandlerHandle)> m_CustomHandlerModified;
    boost::signal<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> m_CustomHandlerParamCreated;
    boost::signal<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> m_CustomHandlerParamDeleted;
    boost::signal<void(Qt3DSDMHandlerParamHandle)> m_CustomHandlerParamModified;
    boost::signal<void(Qt3DSDMInstanceHandle, const TCharStr &)> m_CustomReferencesModified;

    ISlideSystemSignalProvider *m_SlideSystemSignalProvider;

public:
    CStudioFullSystemSignaller(ISlideSystemSignalProvider *inProvider)
        : m_SlideSystemSignalProvider(inProvider)
    {
    }

    virtual TSignalConnectionPtr ConnectChangeSetBegin(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL(m_ChangeSetBegin);
    }
    virtual TSignalConnectionPtr ConnectChangeSetEnd(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL(m_ChangeSetEnd);
    }

    // Used when people start to set component times.
    virtual TSignalConnectionPtr ConnectAnimationSetBegin(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL(m_AnimationBegin);
    }
    virtual TSignalConnectionPtr ConnectAnimationSetEnd(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL(m_AnimationEnd);
    }

    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideCreated);
    }
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectSlideRearranged(const std::function<void(Qt3DSDMSlideHandle, int, int)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_SlideRearranged);
    }
    virtual TSignalConnectionPtr
    ConnectBeginComponentSeconds(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_BeginComponentSeconds);
    }
    virtual TSignalConnectionPtr
    ConnectComponentSeconds(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_ComponentSeconds);
    }
    TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_PropertyLinked);
    }
    TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_PropertyUnlinked);
    }
    TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_ActiveSlide);
    }

    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstanceCreated);
    }
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstanceDeleted);
    }

    virtual TSignalConnectionPtr
    ConnectAnimationCreated(const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_AnimationCreated);
    }
    virtual TSignalConnectionPtr
    ConnectAnimationDeleted(const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_AnimationDeleted);
    }
    TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_KeyframeInserted);
    }
    TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_KeyframeErased);
    }
    virtual TSignalConnectionPtr
    ConnectKeyframeUpdated(const std::function<void(Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_KeyframeUpdated);
    }
    TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_InstancePropertyValue);
    }
    TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(Qt3DSDMAnimationHandle, bool)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_ConnectFirstKeyframeDynamicSet);
    }

    TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_ActionCreated);
    }
    TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL(m_ActionDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectTriggerObjectSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_TriggerObjectSet);
    }
    virtual TSignalConnectionPtr
    ConnectTargetObjectSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_TargetObjectSet);
    }
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_EventHandleSet);
    }
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_HandlerHandleSet);
    }
    virtual TSignalConnectionPtr
    ConnectHandlerArgumentValueSet(const std::function<void(Qt3DSDMHandlerArgHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_HandlerArgumentValueSet);
    }

    TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomPropertyCreated);
    }
    TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomPropertyDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomPropertyModified(const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomPropertyModified);
    }
    TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomEventCreated);
    }
    TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomEventDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(Qt3DSDMEventHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomEventModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerParamCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerParamDeleted);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(Qt3DSDMHandlerParamHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomHandlerParamModified);
    }
    TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) override
    {
        return CONNECT_SIGNAL(m_CustomReferencesModified);
    }

    virtual void SendChangeSetBegin()
    {
        CHECK_SIGNALS_ENABLED();
        m_ChangeSetBegin();
    }
    virtual void SendChangeSetEnd()
    {
        CHECK_SIGNALS_ENABLED();
        m_ChangeSetEnd();
    }

    virtual void SendAnimationSetBegin()
    {
        CHECK_SIGNALS_ENABLED();
        m_AnimationBegin();
    }
    virtual void SendAnimationSetEnd()
    {
        CHECK_SIGNALS_ENABLED();
        m_AnimationEnd();
    }

    void SendSlideCreated(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideCreated(inSlide);
    }
    void SendSlideDeleted(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideDeleted(inSlide);
    }
    void SendSlideRearranged(Qt3DSDMSlideHandle inMaster, int inOldIndex, int inNewIndex) override
    {
        CHECK_SIGNALS_ENABLED();
        m_SlideRearranged(inMaster, inOldIndex, inNewIndex);
    }
    void SendBeginComponentSeconds(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        m_BeginComponentSeconds(inMaster);
    }
    void SendComponentSeconds(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ComponentSeconds(inMaster);
    }
    void SendPropertyLinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        m_PropertyLinked(inMaster, inInstance, inProperty);
    }
    void SendPropertyUnlinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        m_PropertyUnlinked(inMaster, inInstance, inProperty);
    }
    void SendActiveSlide(Qt3DSDMSlideHandle inMaster, int inIndex, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ActiveSlide(inMaster, inIndex, inSlide);
    }

    void SendInstanceCreated(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceCreated(inInstance);
    }
    void SendInstanceDeleted(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstanceDeleted(inInstance);
    }

    void SendAnimationCreated(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        m_AnimationCreated(inAnimation, inInstance, inProperty);
    }
    void SendAnimationDeleted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        m_AnimationDeleted(inAnimation, inInstance, inProperty);
    }
    void SendKeyframeInserted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        m_KeyframeInserted(inAnimation, inKeyframe);
    }
    void SendKeyframeErased(Qt3DSDMAnimationHandle inAnimation,
                                    Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        m_KeyframeErased(inAnimation, inKeyframe);
    }
    void SendKeyframeUpdated(Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        m_KeyframeUpdated(inKeyframe);
    }
    void SendConnectFirstKeyframeDynamicSet(Qt3DSDMAnimationHandle inAnimation,
                                                    bool inDynamic) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ConnectFirstKeyframeDynamicSet(inAnimation, inDynamic);
    }

    void SendInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                           Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        m_InstancePropertyValue(inInstance, inProperty);
    }

    void SendActionCreated(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ActionCreated(inAction, inSlide, inOwner);
    }
    void SendActionDeleted(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_ActionDeleted(inAction, inSlide, inOwner);
    }
    void SendTriggerObjectSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        m_TriggerObjectSet(inAction);
    }
    void SendTargetObjectSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        m_TargetObjectSet(inAction);
    }
    void SendEventSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        m_EventHandleSet(inAction);
    }
    void SendHandlerSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        m_HandlerHandleSet(inAction);
    }
    void SendHandlerArgumentValueSet(Qt3DSDMHandlerArgHandle inHandlerArgument) override
    {
        CHECK_SIGNALS_ENABLED();
        m_HandlerArgumentValueSet(inHandlerArgument);
    }

    void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomPropertyCreated(inProp, inOwner);
    }
    void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomPropertyDeleted(inProp, inOwner);
    }
    void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomPropertyModified(inProp);
    }
    void SendCustomEventCreated(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomEventCreated(inEvent, inOwner);
    }
    void SendCustomEventDeleted(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomEventDeleted(inEvent, inOwner);
    }
    void SendCustomEventModified(Qt3DSDMEventHandle inEvent) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomEventModified(inEvent);
    }
    void SendCustomHandlerCreated(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerCreated(inHandler, inOwner);
    }
    void SendCustomHandlerDeleted(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerDeleted(inHandler, inOwner);
    }
    void SendCustomHandlerModified(Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerModified(inHandler);
    }
    void SendCustomHandlerParamCreated(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerParamCreated(inParameter, inHandler);
    }
    void SendCustomHandlerParamDeleted(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerParamDeleted(inParameter, inHandler);
    }
    void SendCustomHandlerParamModified(Qt3DSDMHandlerParamHandle inParameter) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomHandlerParamModified(inParameter);
    }
    void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) override
    {
        CHECK_SIGNALS_ENABLED();
        m_CustomReferencesModified(inOwner, inString);
    }
};

TSignalItemPtr
CreateStudioFullSystemSignaller(ISlideSystemSignalProvider *inSlideSystemSignalProvider)
{
    return TSignalItemPtr(static_cast<IStudioFullSystemSignalProvider *>(
        new CStudioFullSystemSignaller(inSlideSystemSignalProvider)));
}
}
