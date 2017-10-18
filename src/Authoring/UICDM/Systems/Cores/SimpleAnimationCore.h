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
#ifndef ANIMATIONCOREH
#define ANIMATIONCOREH
#include "UICDMAnimation.h"
#include "HandleSystemBase.h"
#include <boost/unordered_map.hpp>

namespace qt3dsdm {
struct SAnimationTrack : public CHandleObject
{
    int m_Slide;
    int m_Instance;
    int m_Property;
    EAnimationType m_AnimationType;
    size_t m_Index;
    TKeyframeHandleList m_Keyframes;
    bool m_KeyframesDirty;
    bool m_FirstKeyframeDynamic;
    bool m_ArtistEdited;

    SAnimationTrack()
        : m_Slide(0)
        , m_Instance(0)
        , m_Property(0)
        , m_AnimationType(EAnimationTypeLinear)
        , m_Index(0)
        , m_KeyframesDirty(false)
        , m_FirstKeyframeDynamic(false)
        , m_ArtistEdited(true)
    {
    }

    SAnimationTrack(int inHandle, CUICDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                    Qt3DSDMPropertyHandle inProperty, size_t inIndex, EAnimationType inAnimationType,
                    bool inFirstKeyframeDynamic, bool inArtistEdited)
        : CHandleObject(inHandle)
        , m_Slide(inSlide)
        , m_Instance(inInstance)
        , m_Property(inProperty)
        , m_AnimationType(inAnimationType)
        , m_Index(inIndex)
        , m_KeyframesDirty(false)
        , m_FirstKeyframeDynamic(inFirstKeyframeDynamic)
        , m_ArtistEdited(inArtistEdited)
    {
    }

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeSAnimationTrack;
    EHandleObjectType GetType() override { return s_Type; }
};

struct SKeyframe : public CHandleObject
{
    TKeyframe m_Keyframe;
    int m_Animation;

    SKeyframe(int inHandle, int inAnimation, const TKeyframe &inKeyframe)
        : CHandleObject(inHandle)
        , m_Keyframe(inKeyframe)
        , m_Animation(inAnimation)
    {
    }

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeSKeyframe;
    EHandleObjectType GetType() override { return s_Type; }
};

class CAnimationCoreProducer;

class CSimpleAnimationCore : public CHandleBase, public IAnimationCore
{
    TStringTablePtr m_StringTable;
    typedef boost::unordered_multimap<std::pair<int, int>, std::shared_ptr<SAnimationTrack>>
        TStateInstanceAnimationMap;
    // state,instance pair map to animation handle to speed up querying if a particular
    // property is animated.
    mutable TStateInstanceAnimationMap m_AnimationMatchesCache;

public: // Use
    friend class CAnimationCoreProducer;
    // We don't use the string table ptr we are constructed with
    // but the testing system needs an unified interface to creating
    // objects
    CSimpleAnimationCore() {}
    CSimpleAnimationCore(TStringTablePtr strTable)
        : m_StringTable(strTable)
    {
    }
    TStringTablePtr GetStringTablePtr() const { return m_StringTable; }

    CUICDMAnimationHandle CreateAnimation(CUICDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                          EAnimationType inAnimationType,
                                          bool inFirstKeyframeDynamic) override;
    void DeleteAnimation(CUICDMAnimationHandle inAnimation) override;
    CUICDMAnimationHandle GetAnimation(CUICDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, size_t inIndex) const override;
    SAnimationInfo GetAnimationInfo(CUICDMAnimationHandle inAnimation) const override;
    void GetAnimations(TAnimationHandleList &outAnimations) const override;
    void GetAnimations(TAnimationInfoList &outAnimations, CUICDMSlideHandle inMaster,
                       CUICDMSlideHandle inSlide) const override;
    void GetSpecificInstanceAnimations(CUICDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                       TAnimationHandleList &outAnimations) override;

    void SetFirstKeyframeDynamic(CUICDMAnimationHandle inAnimation, bool inValue) override;

    // keyframe manipulation
    CUICDMKeyframeHandle InsertKeyframe(CUICDMAnimationHandle inAnimation,
                                        const TKeyframe &inKeyframe) override;
    void EraseKeyframe(CUICDMKeyframeHandle inKeyframe) override;
    void DeleteAllKeyframes(CUICDMAnimationHandle inAnimation) override;
    CUICDMAnimationHandle GetAnimationForKeyframe(CUICDMKeyframeHandle inKeyframe) const override;
    TKeyframe GetKeyframeData(CUICDMKeyframeHandle inKeyframe) const override;
    void SetKeyframeData(CUICDMKeyframeHandle inKeyframe, const TKeyframe &inData) override;
    // Set the keyframe data, but don't set the artist edited flag.  Used for undo/redo operations
    // where the artist edited flag has handeled by a different transaction
    void DoSetKeyframeData(CUICDMKeyframeHandle inKeyframe, const TKeyframe &inData);
    void GetKeyframes(CUICDMAnimationHandle inAnimation, TKeyframeHandleList &outKeyframes) const override;
    size_t GetKeyframeCount(CUICDMAnimationHandle inAnimation) const override;
    bool IsFirstKeyframe(CUICDMKeyframeHandle inKeyframe) const override;
    // Only implemented in the producer for now.
    void OffsetAnimations(CUICDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                          long inOffset) override;

    void SetIsArtistEdited(CUICDMAnimationHandle inAnimation, bool inEdited = true) override;
    bool IsArtistEdited(CUICDMAnimationHandle inAnimation) const override;

    // Animation Evaluation.
    float EvaluateAnimation(CUICDMAnimationHandle inAnimation, float inSeconds) const override;

    bool KeyframeValid(CUICDMKeyframeHandle inKeyframe) const override;
    bool AnimationValid(CUICDMAnimationHandle inAnimation) const override;

    // Only implemented at the producer level, not at the simple core level.
    void CopyAnimations(CUICDMSlideHandle /*inSourceSlide*/,
                        Qt3DSDMInstanceHandle /*inSourceInstance*/,
                        CUICDMSlideHandle /*inDestSlide*/, Qt3DSDMInstanceHandle /*inDestInstance*/) override
    {
        throw AnimationNotFound(L"");
    }

    // Lookup cache management so we can find particular animations quickly.
    void ClearAnimationMatchesLookupCache() const { m_AnimationMatchesCache.clear(); }
    void AddAnimationToLookupCache(CUICDMAnimationHandle inAnimation) const;
    void RemoveAnimationFromLookupCache(CUICDMAnimationHandle inAnimation) const;
    void AddAnimationToLookupCache(std::shared_ptr<SAnimationTrack> inAnimation) const;
    void RemoveAnimationFromLookupCache(std::shared_ptr<SAnimationTrack> inAnimation) const;

    void EnsureAnimationCache() const;

    CUICDMAnimationHandle CreateAnimationWithHandle(int inHandle, CUICDMSlideHandle inSlide,
                                                    Qt3DSDMInstanceHandle inInstance,
                                                    Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                                    EAnimationType inAnimationType,
                                                    bool inFirstKeyframeDynamic);

    static SAnimationTrack *GetAnimationNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SAnimationTrack *>(
            GetAnimationNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SAnimationTrack *GetAnimationNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const SAnimationTrack *theAnimation = GetHandleObject<SAnimationTrack>(inHandle, inObjects);
        if (theAnimation)
            return theAnimation;
        throw AnimationNotFound(L"");
    }

    static SKeyframe *GetKeyframeNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SKeyframe *>(
            GetKeyframeNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SKeyframe *GetKeyframeNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const SKeyframe *theItem = GetHandleObject<SKeyframe>(inHandle, inObjects);
        if (theItem)
            return theItem;
        throw AnimationKeyframeNotFound(L"");
    }
};

typedef std::shared_ptr<CSimpleAnimationCore> TSimpleAnimationCorePtr;
}

#endif
