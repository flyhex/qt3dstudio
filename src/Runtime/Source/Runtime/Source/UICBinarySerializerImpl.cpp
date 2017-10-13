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

//==============================================================================
//	Includes
//==============================================================================
#include "UICBinarySerializerImpl.h"
#include "UICIPresentation.h"
#include "UICPresentation.h"
#include "UICIScriptBridge.h"
#include "UICAttributeHashes.h"
#include "UICFileStream.h"
#include "foundation/IOStreams.h"
#include "UICCommandEventTypes.h"
#include "UICSceneManager.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "foundation/FileTools.h"
#include "UICApplication.h"
#include "UICRuntimeFactory.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "foundation/SerializationTypes.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "UICSlideSystem.h"
#include "UICLogicSystem.h"
#include "UICParametersSystem.h"

using qt3ds::foundation::atomicIncrement;
using qt3ds::foundation::atomicDecrement;
using qt3ds::QT3DSI32;
using qt3ds::runtime::ISlideSystem;
using qt3ds::foundation::SDataReader;

//==============================================================================
//	Namespace
//==============================================================================
namespace {

using namespace qt3ds::foundation;
using namespace qt3ds;
using namespace qt3ds::runtime;
using Q3DStudio::CFileStream;

struct SSerializerDataReadWrapper : public SDataReader
{
    SSerializerDataReadWrapper(qt3ds::QT3DSU8 *inBegin, qt3ds::QT3DSU8 *inEnd)
        : SDataReader(inBegin, inEnd)
    {
    }

    template <typename TDataType>
    void ReadCopy(TDataType &dt)
    {
        dt = LoadRef<TDataType>();
    }

    template <typename TDataType>
    void ReadRawCopy(TDataType *dt, Q3DStudio::UINT32 inByteCount)
    {
        MemCopy((QT3DSU8 *)dt, inByteCount);
    }
};

void SaveToBinary(IOutStream &inStream, IAnimationSystem &inSystem, IElementAllocator &inAlloc)
{
    inSystem.SaveBinaryData(inStream, inAlloc);
}

void SaveToBinary(IOutStream &inStream, ILogicSystem &inSystem, IElementAllocator & /*inAlloc*/)
{
    inSystem.SaveBinaryData(inStream);
}

void SaveToBinary(IOutStream &inStream, ISlideSystem &inSystem, IElementAllocator & /*inAlloc*/)
{
    inSystem.SaveBinaryData(inStream);
}

void SaveToBinary(IOutStream &inStream, IParametersSystem &inSystem,
                  IElementAllocator & /*inAlloc*/)
{
    inSystem.SaveBinaryData(inStream);
}

template <typename TSystemType>
void SaveSystemToBinary(CFileStream &inStream, TSystemType &inSystem,
                        NVFoundationBase &inFoundation, IElementAllocator &inAlloc)
{
    qt3ds::foundation::CMemorySeekableIOStream theMemStream(inFoundation.getAllocator(), "SaveData");
    SaveToBinary(theMemStream, inSystem, inAlloc);
    inStream.Write((Q3DStudio::UINT32)theMemStream.GetLength());
    inStream.WriteRaw(theMemStream.begin(), (Q3DStudio::UINT32)theMemStream.GetLength());
}

void LoadFromBinary(IAnimationSystem &inSystem, NVDataRef<QT3DSU8> inData,
                    NVDataRef<QT3DSU8> /*inStrTable*/, size_t inElemOffset)
{
    inSystem.LoadBinaryData(inData, inElemOffset);
}

void LoadFromBinary(ILogicSystem &inSystem, NVDataRef<QT3DSU8> inData, NVDataRef<QT3DSU8> /*inStrTable*/,
                    size_t /*inElemOffset*/)
{
    inSystem.LoadBinaryData(inData);
}

void LoadFromBinary(ISlideSystem &inSystem, NVDataRef<QT3DSU8> inData, NVDataRef<QT3DSU8> inStrTable,
                    size_t inElemOffset)
{
    inSystem.LoadBinaryData(inData, inStrTable, inElemOffset);
}

void LoadFromBinary(IParametersSystem &inSystem, NVDataRef<QT3DSU8> inData,
                    NVDataRef<QT3DSU8> /*inStrTable*/, size_t /*inElemOffset*/)
{
    inSystem.LoadBinaryData(inData);
}

template <typename TSystemType>
void LoadSystemFromBinary(SSerializerDataReadWrapper &inWrapper, TSystemType &inSystem,
                          NVDataRef<QT3DSU8> inStrTable, size_t inElemOffset)
{
    QT3DSU32 byteCount = inWrapper.LoadRef<QT3DSU32>();
    NVDataRef<qt3ds::QT3DSU8> theData = toDataRef(inWrapper.m_CurrentPtr, byteCount);
    inWrapper.m_CurrentPtr += byteCount;
    LoadFromBinary(inSystem, theData, inStrTable, inElemOffset);
}

struct SLoadScript
{
    qt3ds::runtime::element::SElement *m_Element;
    QT3DSU32 m_ScriptHandle;
    SLoadScript()
        : m_Element(NULL)
    {
    }
    SLoadScript(qt3ds::runtime::element::SElement *inElement, QT3DSU32 inStrHandle)
        : m_Element(inElement)
        , m_ScriptHandle(inStrHandle)
    {
    }
};

struct SBinarySerializerAppRunnable : public qt3ds::runtime::IAppRunnable
{
    qt3ds::NVFoundationBase &m_Foundation;
    Q3DStudio::IScriptBridge &m_Bridge;
    Q3DStudio::CPresentation &m_Presentation;
    eastl::vector<SLoadScript> m_Scripts;
    size_t m_ElemOffset;
    qt3ds::QT3DSI32 mRefCount;

    SBinarySerializerAppRunnable(qt3ds::NVFoundationBase &fnd, Q3DStudio::IScriptBridge &inBridge,
                                 Q3DStudio::CPresentation &inPresentation, size_t inElemOffset)
        : m_Foundation(fnd)
        , m_Bridge(inBridge)
        , m_Presentation(inPresentation)
        , m_ElemOffset(inElemOffset)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void Run() override
    {
        using qt3ds::runtime::element::SElement;
        qt3ds::foundation::IStringTable &theStrTable(m_Presentation.GetStringTable());
        for (QT3DSU32 idx = 0, end = m_Scripts.size(); idx < end; ++idx) {
            SElement *theElement = reinterpret_cast<SElement *>(
                reinterpret_cast<QT3DSU8 *>(m_Scripts[idx].m_Element) + m_ElemOffset);
            m_Bridge.LoadScript(&m_Presentation, theElement,
                                theStrTable.HandleToStr(m_Scripts[idx].m_ScriptHandle));
        }
    }
};

void SaveScripts(eastl::vector<SLoadScript> &presentationScripts,
                 qt3ds::runtime::element::SElement &inElement, CRegisteredString inSrcPathStr,
                 IElementAllocator &inAlloc)
{
    if (inElement.HasScriptCallbacks()) {
        Q3DStudio::UVariant *theVariant = inElement.FindPropertyValue(inSrcPathStr);
        if (theVariant) {
            presentationScripts.push_back(SLoadScript(inAlloc.GetRemappedElementAddress(&inElement),
                                                      theVariant->m_StringHandle));
        }
    }
    for (element::SElement *theChild = inElement.GetChild(); theChild;
         theChild = theChild->m_Sibling)
        SaveScripts(presentationScripts, *theChild, inSrcPathStr, inAlloc);
}
}

namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 */
CBinarySerializer::CBinarySerializer()
{
}

//==============================================================================
/**
 *	Destructor
 */
CBinarySerializer::~CBinarySerializer()
{
}

void CBinarySerializer::BinarySave(CPresentation *inPresentation)
{
    // Serialize Runtime Portion
    eastl::string thePath(inPresentation->GetFilePath().toLatin1().constData());
    qt3ds::foundation::CFileTools::AppendDirectoryInPathToFile(thePath, "binary");
    qt3ds::foundation::CFileTools::SetExtension(thePath, "uib");
    CFileStream theStream(thePath.c_str(), "wb");
    if (theStream.IsOpen() == false) {
        QT3DS_ASSERT(false);
    }

    SPresentationSize theSize = inPresentation->GetSize();
    theStream.Write(theSize.m_Width);
    theStream.Write(theSize.m_Height);
    theStream.Write(theSize.m_ScaleMode);

    NVFoundationBase &theFoundation =
        inPresentation->GetApplication().GetRuntimeFactoryCore().GetFoundation();
    qt3ds::runtime::IElementAllocator &theAllocator =
        inPresentation->GetApplication().GetElementAllocator();

    SaveSystemToBinary(theStream, inPresentation->GetAnimationSystem(), theFoundation,
                       theAllocator);
    SaveSystemToBinary(theStream, inPresentation->GetLogicSystem(), theFoundation, theAllocator);
    SaveSystemToBinary(theStream, inPresentation->GetSlideSystem(), theFoundation, theAllocator);
    SaveSystemToBinary(theStream, inPresentation->GetParametersSystem(), theFoundation,
                       theAllocator);
    eastl::vector<SLoadScript> theScripts;
    SaveScripts(theScripts, *inPresentation->GetRoot(),
                inPresentation->GetStringTable().RegisterStr("behaviorscripts"), theAllocator);
    theStream.Write((QT3DSU32)theScripts.size());
    if (!theScripts.empty())
        theStream.WriteRaw(theScripts.data(), theScripts.size() * sizeof(SLoadScript));

    theStream.Close();
}

BOOL CBinarySerializer::BinaryLoad(CPresentation *inPresentation, size_t inElementMemoryOffset,
                                   qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8> inPresentationFile,
                                   qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8> inStringTableData,
                                   qt3ds::foundation::IPerfTimer &inPerfTimer)
{
    SSerializerDataReadWrapper theStream(inPresentationFile.begin(), inPresentationFile.end());
    SPresentationSize theSize;
    theStream.ReadCopy(theSize.m_Width);
    theStream.ReadCopy(theSize.m_Height);
    theStream.ReadCopy(theSize.m_ScaleMode);
    inPresentation->SetSize(theSize);
    {
        SStackPerfTimer __perfTimer(inPerfTimer, "Load UIB - AnimationBuilder");
        LoadSystemFromBinary(theStream, inPresentation->GetAnimationSystem(), inStringTableData,
                             inElementMemoryOffset);
    }
    {
        SStackPerfTimer __perfTimer(inPerfTimer, "Load UIB - LogicBuilder");
        LoadSystemFromBinary(theStream, inPresentation->GetLogicSystem(), inStringTableData,
                             inElementMemoryOffset);
    }
    {
        SStackPerfTimer __perfTimer(inPerfTimer, "Load UIB - ParametersBuilder");
        LoadSystemFromBinary(theStream, inPresentation->GetSlideSystem(), inStringTableData,
                             inElementMemoryOffset);
    }
    {
        SStackPerfTimer __perfTimer(inPerfTimer, "Load UIB - SlideBuilder");
        LoadSystemFromBinary(theStream, inPresentation->GetParametersSystem(), inStringTableData,
                             inElementMemoryOffset);
    }
    QT3DSU32 numScripts = theStream.LoadRef<QT3DSU32>();
    if (numScripts) {
        NVFoundationBase &theFoundation(
            inPresentation->GetApplication().GetRuntimeFactoryCore().GetFoundation());
        SBinarySerializerAppRunnable &theAppRunnable =
            *QT3DS_NEW(theFoundation.getAllocator(),
                    SBinarySerializerAppRunnable)(theFoundation, *inPresentation->GetScriptBridge(),
                                                  *inPresentation, inElementMemoryOffset);
        theAppRunnable.m_Scripts.resize(numScripts);
        theStream.ReadRawCopy(theAppRunnable.m_Scripts.data(),
                              theAppRunnable.m_Scripts.size() * sizeof(SLoadScript));
        inPresentation->GetApplication().QueueForMainThread(theAppRunnable);
    }
    return true;
}

IBinarySerializer &IBinarySerializer::Create()
{
    CBinarySerializer &retval = *new CBinarySerializer();
    return retval;
}

} // namespace Q3DStudio
