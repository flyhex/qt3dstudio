/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#include "Qt3DSStateInputStreamFactory.h"

#include "stdio.h"

#include "foundation/Qt3DSLogging.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSMutex.h"
#include <QFile>

using namespace uic::state;
using qt3ds::foundation::CFileTools;

namespace {

using qt3ds::foundation::atomicIncrement;
using qt3ds::foundation::atomicDecrement;
using qt3ds::QT3DSI32;

struct SInputStream : public IRefCountedInputStream
{
    qt3ds::NVFoundationBase &m_Foundation;
    eastl::string m_Path;
    QFile m_File;
    volatile qt3ds::QT3DSI32 mRefCount;

    SInputStream(qt3ds::NVFoundationBase &inFoundation, const eastl::string &inPath)
        : m_Foundation(inFoundation)
        , m_Path(inPath)
        , m_File(inPath.c_str())
        , mRefCount(0)
    {
        m_File.open(QIODevice::ReadOnly);
    }
    virtual ~SInputStream()
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    qt3ds::QT3DSU32 Read(qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8> data) override
    {
        return m_File.read((char *)data.begin(), data.size());
    }

    bool Write(qt3ds::foundation::NVConstDataRef<qt3ds::QT3DSU8> /*data*/) override
    {
        QT3DS_ASSERT(false);
        return false;
    }

    void SetPosition(qt3ds::QT3DSI64 inOffset, qt3ds::foundation::SeekPosition::Enum inEnum) override
    {
        if (inOffset > QT3DS_MAX_I32 || inOffset < QT3DS_MIN_I32) {
            qCCritical(qt3ds::INVALID_OPERATION, "Attempt to seek further than platform allows");
            QT3DS_ASSERT(false);
            return;
        } else {
            CFileTools::SetStreamPosition(m_File, inOffset, inEnum);
        }
    }
    qt3ds::QT3DSI64 GetPosition() const override
    {
        return m_File.pos();
    }

    static SInputStream *OpenFile(const char8_t *inPath, qt3ds::NVFoundationBase &inFoundation)
    {
        eastl::string finalPath = CFileTools::GetFileOrAssetPath(inPath);
        QFile tmp(finalPath.c_str());
        if (tmp.exists())
            return QT3DS_NEW(inFoundation.getAllocator(), SInputStream)(inFoundation, finalPath);
        return NULL;
    }
};

typedef eastl::basic_string<char8_t, qt3ds::foundation::ForwardingAllocator> TStrType;
struct SFactory : public IInputStreamFactory
{
    qt3ds::NVFoundationBase &m_Foundation;
    volatile qt3ds::QT3DSI32 mRefCount;
    TStrType m_SearchString;
    qt3ds::foundation::nvvector<TStrType> m_SearchPaths;
    TStrType m_TempAddSearch;

    qt3ds::foundation::Mutex m_Mutex;
    typedef qt3ds::foundation::Mutex::ScopedLock TScopedLock;

    SFactory(qt3ds::NVFoundationBase &inFoundation)
        : m_Foundation(inFoundation)
        , mRefCount(0)
        , m_SearchString(qt3ds::foundation::ForwardingAllocator(inFoundation.getAllocator(),
                                                             "SFactory::m_SearchString"))
        , m_SearchPaths(inFoundation.getAllocator(), "SFactory::m_SearchPaths")
        , m_TempAddSearch(qt3ds::foundation::ForwardingAllocator(inFoundation.getAllocator(),
                                                              "SFactory::m_TempAddSearch"))
        , m_Mutex(inFoundation.getAllocator())
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void AddSearchDirectory(const char8_t *inDirectory) override
    {
        TScopedLock __factoryLocker(m_Mutex);
        if (inDirectory && *inDirectory) {
            bool foundPath = false;
            m_TempAddSearch.assign(inDirectory);
            qt3ds::foundation::CFileTools::NormalizePath(m_TempAddSearch);
            for (qt3ds::QT3DSU32 idx = 0, end = m_SearchPaths.size(); idx < end && foundPath == false;
                 ++idx) {
                foundPath = m_SearchPaths[idx].compare(m_TempAddSearch.c_str()) == 0;
            }
            if (!foundPath) {
                qCInfo(qt3ds::TRACE_INFO, "Adding search path: %s", inDirectory);
                m_SearchPaths.push_back(m_TempAddSearch);
            }
        }
    }
    void AttemptCFileOpen()
    {
        qCInfo(qt3ds::TRACE_INFO, "attempting C file api for %s", m_SearchString.c_str());
        FILE *theTest = fopen(m_SearchString.c_str(), "rb");
        if (theTest) {
            qCInfo(qt3ds::TRACE_INFO, "C file api succeeded for %s", m_SearchString.c_str());
            fclose(theTest);
        } else {
            qCInfo(qt3ds::TRACE_INFO, "C file api failed for %s", m_SearchString.c_str());
        }
    }

    // Remove the ./ from the relative path as this allows APK lookup to succeed
    static void CheckRelative(TStrType &ioStr)
    {
        if (ioStr.find("./") == 0)
            ioStr.erase(ioStr.begin(), ioStr.begin() + 2);
    }

    IRefCountedInputStream *GetStreamForFile(const char8_t *inFilename, bool inQuiet) override
    {
        TScopedLock __factoryLocker(m_Mutex);
        SInputStream *theFile = NULL;
        for (qt3ds::QT3DSU32 idx = 0, end = m_SearchPaths.size() + 1; theFile == NULL && idx < end;
             ++idx) {
            if (idx) {
                qt3ds::foundation::CFileTools::CombineBaseAndRelative(m_SearchPaths[idx - 1].c_str(),
                                                                   inFilename, m_SearchString);
            } else {
                m_SearchString.assign(inFilename);
            }
            qt3ds::foundation::CFileTools::ToPlatformPath(m_SearchString);
            CheckRelative(m_SearchString);
            theFile = SInputStream::OpenFile(m_SearchString.c_str(), m_Foundation);
        }
        if (theFile) {
            qCInfo(qt3ds::TRACE_INFO, "file %s resolved to: %s", inFilename, m_SearchString.c_str());
        } else {
            if (inQuiet == false) {
                // Print extensive debugging information.
                qCWarning(qt3ds::WARNING, "Failed to find file: %s", inFilename);
                qCWarning(qt3ds::WARNING, "Searched paths: %s", inFilename);
                m_SearchString.assign(inFilename);
                qt3ds::foundation::CFileTools::ToPlatformPath(m_SearchString);
                qCWarning(qt3ds::WARNING, "%s", m_SearchString.c_str());
                for (qt3ds::QT3DSU32 idx = 0, end = m_SearchPaths.size(); idx < end; ++idx) {
                    qt3ds::foundation::CFileTools::CombineBaseAndRelative(m_SearchPaths[idx].c_str(),
                                                                       inFilename, m_SearchString);
                    qt3ds::foundation::CFileTools::ToPlatformPath(m_SearchString);
                    CheckRelative(m_SearchString);
                    if (m_SearchString.compare(inFilename) != 0)
                        qCWarning(qt3ds::WARNING, "%s", m_SearchString.c_str());
                }
            }
        }
        return theFile;
    }

    bool GetPathForFile(const char8_t *inFilename, eastl::string &outFile,
                                bool inQuiet = false) override
    {
        qt3ds::foundation::NVScopedRefCounted<IRefCountedInputStream> theStream =
            GetStreamForFile(inFilename, inQuiet);
        if (theStream) {
            SInputStream *theRealStream = static_cast<SInputStream *>(theStream.mPtr);
            outFile = theRealStream->m_Path;
            return true;
        }
        return false;
    }
};
}

IInputStreamFactory &IInputStreamFactory::Create(qt3ds::NVFoundationBase &inFoundation)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SFactory)(inFoundation);
}
