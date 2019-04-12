/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "RuntimePrefix.h"
#include "Qt3DSSlideSystem.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSAnimationSystem.h"
#include "Qt3DSLogicSystem.h"
#include "foundation/SerializationTypes.h"
#include "foundation/IOStreams.h"
#include "Qt3DSHash.h"
#include "Qt3DSTimePolicy.h"
#include "foundation/Qt3DSIndexableLinkedList.h"

using namespace qt3ds::runtime;
using namespace qt3ds::runtime::element;

namespace {
struct SSlideAttribute
{
    QT3DSU32 m_Index;
    Q3DStudio::UVariant m_Value;
    SSlideAttribute()
        : m_Index(0)
    {
        m_Value.m_INT32 = 0;
    }
    SSlideAttribute(QT3DSU32 idx, Q3DStudio::UVariant val)
        : m_Index(idx)
        , m_Value(val)
    {
    }
};

struct SSlideAttributeNode
{
    enum {
        AttributeCount = 8,
    };

    SSlideAttribute m_Data[AttributeCount];
    SSlideAttributeNode *m_NextNode;

    SSlideAttributeNode()
        : m_NextNode(NULL)
    {
    }
};

typedef IndexableLinkedList<SSlideAttributeNode, SSlideAttribute,
                            SSlideAttributeNode::AttributeCount>
    TSlideAttributeNodeList;

struct SSlideElement
{
    QT3DSU32 m_ElementHandle;
    QT3DSU32 m_AttributeCount : 31;
    QT3DSU32 m_Active : 1;
    SSlideElement *m_NextElement;
    SSlideAttributeNode *m_AttributeNodes;

    SSlideElement()
        : m_ElementHandle(0)
        , m_AttributeCount(0)
        , m_Active(false)
        , m_NextElement(NULL)
        , m_AttributeNodes(NULL)
    {
    }
};

struct SSlideAnimActionNode
{
    enum {
        AnimActionCount = 8,
    };
    SSlideAnimAction m_Data[AnimActionCount];
    SSlideAnimActionNode *m_NextNode;
    SSlideAnimActionNode()
        : m_NextNode(NULL)
    {
    }
};

typedef IndexableLinkedList<SSlideAnimActionNode, SSlideAnimAction,
                            SSlideAnimActionNode::AnimActionCount>
    TSlideAnimActionNodeList;

struct SSlide
{
    SSlide *m_NextSlide;
    CRegisteredString m_Name;
    QT3DSU32 m_PlayMode : 3;
    QT3DSU32 m_PlayThroughTo : 8; // OXFF means no playthrough
    QT3DSU32 m_Paused : 1;
    QT3DSU32 m_StartTime;
    QT3DSU32 m_EndTime;
    QT3DSU32 m_AnimActionCount;
    SSlideElement *m_FirstElement;
    SSlideAnimActionNode *m_FirstAnimActionNode;

    SSlide()
        : m_NextSlide(NULL)
        , m_PlayMode(PlayMode::StopAtEnd)
        , m_PlayThroughTo(0xFF)
        , m_Paused(false)
        , m_StartTime(0)
        , m_EndTime(0)
        , m_AnimActionCount(0)
        , m_FirstElement(NULL)
        , m_FirstAnimActionNode(NULL)
    {
    }

    void Initialize(CRegisteredString inName, PlayMode::Enum inMode, QT3DSU8 inPlayThrough,
                    bool inPaused, QT3DSU32 inStartTime, QT3DSU32 inEndTime)
    {
        m_Name = inName;
        m_PlayMode = inMode;
        m_PlayThroughTo = inPlayThrough;
        m_Paused = inPaused;
        m_StartTime = inStartTime;
        m_EndTime = inEndTime;
    }
};

struct SSlideSystem : public ISlideSystem
{
    typedef nvhash_map<SElement *, SSlide *> TComponentSlideHash;
    typedef Pool<SSlideAttributeNode, ForwardingAllocator> TAttributeNodePool;
    typedef Pool<SSlideAnimActionNode, ForwardingAllocator> TSlideAnimActionPool;
    typedef Pool<SSlideElement, ForwardingAllocator> TSlideElementPool;
    typedef Pool<SSlide, ForwardingAllocator> TSlidePool;

    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    IElementAllocator &m_ElementSystem;
    TAttributeNodePool m_AttributeNodePool;
    TSlideAnimActionPool m_AnimActionPool;
    TSlideElementPool m_SlideElements;
    TSlidePool m_SlidePool;
    TComponentSlideHash m_Slides;

    SSlide *m_CurrentSlide;
    SSlideElement *m_CurrentSlideElement;

    NVDataRef<QT3DSU8> m_LoadData;

    QT3DSI32 m_RefCount;

    SSlideSystem(NVFoundationBase &inFnd, IStringTable &inStrTable, IElementAllocator &inAllocator)
        : m_Foundation(inFnd)
        , m_StringTable(inStrTable)
        , m_ElementSystem(inAllocator)
        , m_AttributeNodePool(ForwardingAllocator(inFnd.getAllocator(), "m_AttributeNodePool"))
        , m_AnimActionPool(ForwardingAllocator(inFnd.getAllocator(), "m_AnimActionPool"))
        , m_SlidePool(ForwardingAllocator(inFnd.getAllocator(), "m_SlidePool"))
        , m_SlideElements(ForwardingAllocator(inFnd.getAllocator(), "m_SlideElements"))
        , m_Slides(inFnd.getAllocator(), "m_Slides")
        , m_CurrentSlide(NULL)
        , m_CurrentSlideElement(NULL)
        , m_RefCount(0)
    {
    }

    void addRef() override { atomicIncrement(&m_RefCount); }

    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Foundation.getAllocator());
            NVDelete(alloc, this);
        }
    }

    QT3DSU32 AddSlide(element::SElement &inComponent, const char8_t *inName,
                           PlayMode::Enum inPlayMode, bool inPaused, QT3DSU8 inPlayThroughTo,
                           QT3DSU32 inMinTime, QT3DSU32 inMaxTime) override
    {
        eastl::pair<TComponentSlideHash::iterator, bool> inserter =
            m_Slides.insert(eastl::make_pair(&inComponent, (SSlide *)NULL));
        SSlide *newSlide = m_SlidePool.construct(__FILE__, __LINE__);
        QT3DSU32 slideIndex = 0;
        if (inserter.first->second == NULL) {
            inserter.first->second = newSlide;
        } else {
            SSlide *theSlide = NULL;
            for (theSlide = inserter.first->second; theSlide->m_NextSlide != NULL;
                 theSlide = theSlide->m_NextSlide) {
                ++slideIndex;
            }
            theSlide->m_NextSlide = newSlide;
        }
        m_CurrentSlide = newSlide;
        newSlide->Initialize(m_StringTable.RegisterStr(inName), inPlayMode, inPlayThroughTo,
                             inPaused, inMinTime, inMaxTime);
        m_CurrentSlideElement = NULL;
        if (inComponent.IsComponent()) {
            SComponent &theComponent = static_cast<SComponent &>(inComponent);
            theComponent.m_SlideCount++;
        }
        return slideIndex;
    }

    void SetSlideMaxTime(QT3DSU32 inMaxTime) override
    {
        if (m_CurrentSlide != NULL)
            m_CurrentSlide->m_EndTime = inMaxTime;
    }

    void AddSlideElement(element::SElement &inElement, bool inActive) override
    {
        if (m_CurrentSlide != NULL) {
            SSlideElement *lastSlideElement = m_CurrentSlideElement;
            m_CurrentSlideElement = m_SlideElements.construct(__FILE__, __LINE__);
            m_CurrentSlideElement->m_Active = inActive;
            m_CurrentSlideElement->m_ElementHandle = inElement.GetHandle();
            if (lastSlideElement == NULL) {
                QT3DS_ASSERT(m_CurrentSlide->m_FirstElement == NULL);
                m_CurrentSlide->m_FirstElement = m_CurrentSlideElement;
            } else {
                lastSlideElement->m_NextElement = m_CurrentSlideElement;
            }
            m_CurrentSlideElement->m_Active = inActive;
        } else {
            QT3DS_ASSERT(false);
        }
    }
    void AddSlideAttribute(Q3DStudio::SAttributeKey inKey, Q3DStudio::UVariant inValue) override
    {
        if (m_CurrentSlideElement != NULL) {

            SElement *theElement =
                m_ElementSystem.FindElementByHandle(m_CurrentSlideElement->m_ElementHandle);
            Option<QT3DSU32> theIdx = theElement->FindPropertyIndex(inKey.m_Hash);
            if (theIdx.hasValue() == false) {
                QT3DS_ASSERT(false);
                return;
            }
            QT3DSU32 count = m_CurrentSlideElement->m_AttributeCount;
            TSlideAttributeNodeList::Create(m_CurrentSlideElement->m_AttributeNodes, count,
                                            m_AttributeNodePool) =
                SSlideAttribute(*theIdx, inValue);
            m_CurrentSlideElement->m_AttributeCount = count;
        }
    }

    SSlideAnimAction *AddSlideAnimAction(bool inAnimation, QT3DSI32 inId, bool inActive) override
    {
        if (m_CurrentSlide != NULL) {
            SSlideAnimAction &theAnimAction = TSlideAnimActionNodeList::Create(
                m_CurrentSlide->m_FirstAnimActionNode, m_CurrentSlide->m_AnimActionCount,
                m_AnimActionPool);
            theAnimAction = SSlideAnimAction((QT3DSI32)inId, inActive, inAnimation);
            &theAnimAction;
        }

        return NULL;
    }

    const SSlide *FindSlide(SSlideKey inKey) const
    {
        TComponentSlideHash::const_iterator iter = m_Slides.find(inKey.m_Component);
        if (iter == m_Slides.end())
            return NULL;

        SSlide *theSlide = iter->second;
        for (QT3DSU32 idx = inKey.m_Index; idx; --idx) {
            if (theSlide)
                theSlide = theSlide->m_NextSlide;
        }

        return theSlide;
    }

    template <typename TOperator>
    static void IterateSlideAnimActions(const SSlide &inSlide, TOperator inOp)
    {
        for (TSlideAnimActionNodeList::const_iterator
                 iter = TSlideAnimActionNodeList::begin(inSlide.m_FirstAnimActionNode,
                                                        inSlide.m_AnimActionCount),
                 end = TSlideAnimActionNodeList::end(inSlide.m_FirstAnimActionNode,
                                                     inSlide.m_AnimActionCount);
             iter != end; ++iter) {
            inOp(*iter);
        }
    }

    template <typename TOperator>
    static void IterateSlideElementAttributes(const SSlide &inSlide, TOperator inOp,
                                              IElementAllocator &inAlloc)
    {
        for (SSlideElement *theElement = inSlide.m_FirstElement; theElement;
             theElement = theElement->m_NextElement) {
            SElement *theElem = inAlloc.FindElementByHandle(theElement->m_ElementHandle);
            if (!theElem) {
                QT3DS_ASSERT(false);
            } else {
                if (inOp.handleElementActive(*theElem, theElement->m_Active)) {
                    for (TSlideAttributeNodeList::const_iterator
                             iter = TSlideAttributeNodeList::begin(theElement->m_AttributeNodes,
                                                                   theElement->m_AttributeCount),
                             end = TSlideAttributeNodeList::end(theElement->m_AttributeNodes,
                                                                theElement->m_AttributeCount);
                         iter != end; ++iter) {
                        inOp.handleElementAttribute(*theElem, *iter);
                    }
                }
            }
        }
    }

    struct SDynamicKeyOperator
    {
        IAnimationSystem &m_AnimationSystem;
        SDynamicKeyOperator(IAnimationSystem &anim)
            : m_AnimationSystem(anim)
        {
        }
        void operator()(const SSlideAnimAction &theAction)
        {
            if (theAction.m_IsAnimation && theAction.m_Active)
                m_AnimationSystem.UpdateDynamicKey(theAction.m_Id);
        }
    };

    struct SExecuteAnimActionOperator
    {
        IElementAllocator &m_ElementAllocator;
        IAnimationSystem &m_AnimationSystem;
        ILogicSystem &m_LogicManager;
        bool m_Invert;

        SExecuteAnimActionOperator(IElementAllocator &inElemAllocator, IAnimationSystem &anim,
                                   ILogicSystem &logic, bool inInvert = false)
            : m_ElementAllocator(inElemAllocator)
            , m_AnimationSystem(anim)
            , m_LogicManager(logic)
            , m_Invert(inInvert)
        {
        }

        void operator()(const SSlideAnimAction &theAction)
        {
            bool active = theAction.m_Active;
            if (m_Invert)
                active = !active;
            if (theAction.m_IsAnimation)
                m_AnimationSystem.SetActive(theAction.m_Id, active);
            else
                m_LogicManager.SetActive(theAction.m_Id, active, m_ElementAllocator);
        }
    };

    struct SElementOperator
    {
        bool m_InvertActive;
        SElementOperator(bool inInvert = false)
            : m_InvertActive(inInvert)
        {
        }
        bool handleElementActive(SElement &inElement, bool inActive)
        {
            if (m_InvertActive && inElement.Flags().IsExplicitActive())
                inActive = !inActive;
            inElement.Flags().SetExplicitActive(inActive);

            if (m_InvertActive)
                return false;

            return inActive;
        }

        void handleElementAttribute(SElement &inElement, const SSlideAttribute &inAttribute)
        {
            Option<TPropertyDescAndValuePtr> theProperty =
                inElement.GetPropertyByIndex(inAttribute.m_Index);
            if (theProperty.hasValue()) {
                inElement.SetAttribute(*theProperty, inAttribute.m_Value);
            } else {
                QT3DS_ASSERT(false);
            }
        }
    };

    void InitializeDynamicKeys(SSlideKey inKey, IAnimationSystem &inAnimationSystem) const override
    {
        const SSlide *theSlide = FindSlide(inKey);
        if (theSlide != NULL) {
            IterateSlideAnimActions(*theSlide, SDynamicKeyOperator(inAnimationSystem));
        }
    }

    void ExecuteSlide(SSlideKey inKey, IAnimationSystem &inAnimationSystem,
                              ILogicSystem &inLogicManager) const override
    {
        const SSlide *theSlide = FindSlide(inKey);

        if (theSlide == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        if (inKey.m_Component->IsComponent()) {
            Q3DStudio::TTimeUnit theLoopDuration = theSlide->m_EndTime - theSlide->m_StartTime;
            SComponent &theComponent = static_cast<SComponent &>(*inKey.m_Component);
            Q3DStudio::CTimePolicy &thePolicy = theComponent.GetTimePolicy();
            Q3DStudio::TimePolicyModes::Enum theMode = Q3DStudio::TimePolicyModes::StopAtEnd;
            switch (theSlide->m_PlayMode) {
            case PlayMode::Looping:
                theMode = Q3DStudio::TimePolicyModes::Looping;
                break;
            case PlayMode::PingPong:
                theMode = Q3DStudio::TimePolicyModes::PingPong;
                break;
            case PlayMode::Ping:
                theMode = Q3DStudio::TimePolicyModes::Ping;
                break;

            case PlayMode::StopAtEnd:
            case PlayMode::PlayThroughTo:
            default:
                theMode = Q3DStudio::TimePolicyModes::StopAtEnd;
                break;
            }
            thePolicy.Initialize(theLoopDuration, theMode);
            thePolicy.SetPaused(theSlide->m_Paused);

            theComponent.SetPlayThrough(theSlide->m_PlayMode == PlayMode::PlayThroughTo);
        }

        IterateSlideElementAttributes(*theSlide, SElementOperator(false), m_ElementSystem);
        IterateSlideAnimActions(*theSlide, SExecuteAnimActionOperator(
                                               m_ElementSystem, inAnimationSystem, inLogicManager));
    }

    void RollbackSlide(SSlideKey inKey, IAnimationSystem &inAnimationSystem,
                               ILogicSystem &inLogicManager) const override
    {
        const SSlide *theSlide = FindSlide(inKey);

        if (theSlide == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        IterateSlideElementAttributes(*theSlide, SElementOperator(true), m_ElementSystem);
        IterateSlideAnimActions(
            *theSlide,
            SExecuteAnimActionOperator(m_ElementSystem, inAnimationSystem, inLogicManager, true));
    }

    QT3DSU8 FindSlide(element::SElement &inComponent, const char8_t *inSlideName) const override
    {
        TComponentSlideHash::const_iterator theFindResult = m_Slides.find(&inComponent);
        if (theFindResult == m_Slides.end()) {
            QT3DS_ASSERT(false);
            return 0xFF;
        }
        CRegisteredString theRegName = m_StringTable.RegisterStr(inSlideName);
        QT3DSU8 idx = 0;
        for (const SSlide *theSlide = theFindResult->second; theSlide;
             theSlide = theSlide->m_NextSlide, ++idx) {
            if (theSlide->m_Name == theRegName)
                return idx;
        }
        return 0xFF;
    }

    QT3DSU8 FindSlide(element::SElement &inComponent, QT3DSU32 inSlideHashName) const override
    {
        TComponentSlideHash::const_iterator theFindResult = m_Slides.find(&inComponent);
        if (theFindResult == m_Slides.end()) {
            QT3DS_ASSERT(false);
            return 0xFF;
        }

        QT3DSU8 idx = 0;
        for (const SSlide *theSlide = theFindResult->second; theSlide;
             theSlide = theSlide->m_NextSlide, ++idx) {
            if (Q3DStudio::CHash::HashString(theSlide->m_Name) == inSlideHashName)
                return idx;
        }
        return 0xFF;
    }

    const char8_t *GetSlideName(SSlideKey inKey) const override
    {
        const SSlide *theSlide = FindSlide(inKey);
        if (theSlide)
            return theSlide->m_Name.c_str();
        QT3DS_ASSERT(false);
        return "";
    }

    QT3DSU8 GetPlaythroughToSlideIndex(SSlideKey inKey) const override
    {
        const SSlide *theSlide = FindSlide(inKey);
        if (theSlide)
            return theSlide->m_PlayThroughTo;
        QT3DS_ASSERT(false);
        return 0xFF;
    }
};
}

ISlideSystem &ISlideSystem::CreateSlideSystem(NVFoundationBase &inFnd, IStringTable &inStrTable,
                                              IElementAllocator &inElemAllocator)
{
    return *QT3DS_NEW(inFnd.getAllocator(), SSlideSystem)(inFnd, inStrTable, inElemAllocator);
}
