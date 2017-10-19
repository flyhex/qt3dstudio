/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#pragma once
#ifndef DOCUMENTRESOURCEMANAGERLUAPARSERH
#define DOCUMENTRESOURCEMANAGERLUAPARSERH

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "foundation/StrConvertUTF.h"
#include <EASTL/string.h>
#include "Qt3DSRenderInputStreamFactory.h"

namespace Q3DStudio {
namespace LuaParser {

    using namespace qt3dsdm;
    using namespace std;
    using namespace Q3DStudio;

    struct SLuaParserContext
    {
        const char8_t *m_Data;
        const char8_t *m_End;
        QT3DSU32 m_CurrentLineNumber;
        bool m_FoundBlock;
        SLuaParserContext(const char8_t *inData, const char8_t *inEnd)
            : m_Data(inData)
            , m_End(inEnd)
            , m_CurrentLineNumber(1)
            , m_FoundBlock(false)
        {
            QT3DS_ASSERT(inData <= inEnd);
        }

        bool HasMoreInput() const { return m_Data < m_End; }

        wchar_t NextChar()
        {
            wchar_t retval = 0;
            if (HasMoreInput()) {
                do {
                    retval = *m_Data;
                    ++m_Data;
                } while (retval == '\r' && HasMoreInput());
            }
            if (retval == '\n')
                ++m_CurrentLineNumber;
            return retval;
        }

        void ParseBlockComment(eastl::string &outLines)
        {
            const char8_t *firstLineBegin = m_Data;
            m_FoundBlock = true;
            do {
                wchar_t nextChar = NextChar();
                if (nextChar == ']') {
                    nextChar = NextChar();
                    if (nextChar == ']') {
                        outLines.append(firstLineBegin, 0, uint32_t(m_Data - firstLineBegin - 2));
                        return;
                    }
                }
            } while (HasMoreInput());
        }

        void ParseComment(eastl::string &outLines)
        {
            wchar_t nextChar = NextChar();
            if (nextChar == '[') {
                nextChar = NextChar();
                if (nextChar == '[')
                    ParseBlockComment(outLines);
            }
            // empty loop intentional
            for (; nextChar != 0 && nextChar != '\n'; nextChar = NextChar())
                ;
        }

        // Pre parse to strip the input we care about from the rest of the file.
        static void ParseLuaInput(eastl::string &inLuaFile, eastl::string &outLines)
        {
            const char8_t *theEnd = inLuaFile.c_str() + inLuaFile.size();
            SLuaParserContext context(inLuaFile.c_str(), theEnd);
            int dashCount = 0;
            // Note that in the specification we state that only the first lua block
            // comment will be parsed for the behavior information
            while (context.HasMoreInput() && context.m_FoundBlock == false) {
                wchar_t nextValue = context.NextChar();
                if (nextValue == '-') {
                    ++dashCount;
                    if (dashCount == 2) {
                        context.ParseComment(outLines);
                        dashCount = 0;
                    }
                }
            }
        }
    };

    struct StringInStream : qt3ds::foundation::IInStream
    {
        const eastl::string &m_String;
        QT3DSU32 m_Offset;

        StringInStream(const eastl::string &str)
            : m_String(str)
            , m_Offset(0)
        {
        }

        QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
        {
            const char *theStart((const char *)m_String.c_str());
            const char *theEnd(theStart + m_String.size());

            QT3DSU32 amountLeft = QT3DSU32(theEnd - theStart) - m_Offset;
            QT3DSU32 amountToRead = qt3ds::NVMin(data.size(), amountLeft);
            qt3ds::intrinsics::memCopy(data.begin(), theStart + m_Offset, amountToRead);
            m_Offset += amountToRead;
            return amountToRead;
        }
    };

    struct SLuaParser
    {
        static std::shared_ptr<IDOMReader>
        ParseLuaData(std::shared_ptr<IDOMFactory> inFactory,
                     std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                     eastl::string &inFileData, CXmlErrorHandler &inErrorHandler)
        {
            eastl::string theOpenTag("<Behavior>\n");
            SLuaParserContext::ParseLuaInput(inFileData, theOpenTag);
            // Ensure we strip the last "--" in case there is any
            uint32_t theSize = theOpenTag.size();
            uint32_t theddPos = theOpenTag.rfind("--");
            if (theddPos == theSize - 2)
                theOpenTag.erase(theddPos);
            theOpenTag.append("</Behavior>\n");

            // Replace /r/n with /n because the xml parse will interpret /r/n as two lines thus
            // leading to incorrect line numbers.
            for (uint32_t thePos = theOpenTag.find("\r\n"); thePos != eastl::string::npos;
                 thePos = theOpenTag.find("\r\n", thePos))
                theOpenTag = theOpenTag.replace(thePos, 1, "\n");

            StringInStream theStream(theOpenTag);

            SDOMElement *theElem(CDOMSerializer::Read(*inFactory, theStream, &inErrorHandler));
            if (theElem == NULL) {
                return std::shared_ptr<IDOMReader>();
            } else
                return IDOMReader::CreateDOMReader(*theElem, inStringTable, inFactory);
        }

        static wchar_t NarrowToWide(QT3DSU8 inData) { return inData; }

        static std::shared_ptr<IDOMReader>
        ParseLuaFile(std::shared_ptr<IDOMFactory> inFactory,
                     std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                     const char8_t *inFileData, CXmlErrorHandler &inErrorHandler,
                     qt3ds::render::IInputStreamFactory &inStreamFactory)
        {
            eastl::vector<QT3DSU8> readBuf;
            {
                using namespace qt3ds;
                using namespace qt3ds::foundation;
                eastl::string strConvert;
                NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream(
                    inStreamFactory.GetStreamForFile(inFileData));
                if (!theStream) {
                    QT3DS_ASSERT(0);
                    return std::shared_ptr<IDOMReader>();
                }

                QT3DSU32 readLen = 0;
                do {
                    uint32_t offset = readBuf.size();
                    readBuf.resize(offset + 4096);
                    readLen = theStream->Read(NVDataRef<QT3DSU8>(&readBuf[0] + offset, 4096));
                    readBuf.resize(offset + readLen);
                } while (readLen == 4096);
            }
            if (readBuf.size() < 2) {
                QT3DS_ASSERT(false);
                return std::shared_ptr<IDOMReader>();
            }
            eastl::string convertBuf;

            //...sigh..text files.
            // Everything gets converted to UTF-8
            // little endian, big endian UTF-16
            // or ASCII assumed
            QT3DSU32 dataSize = 0;
            QT3DSU16 endianIndicator;
            memcpy(&endianIndicator, &readBuf[0], 2);
            // Detect LE UTF-16 and copy directly into convert buf
            if (endianIndicator == (QT3DSU16)0xFEFF) {
                const char16_t *dataPtr = (const char16_t *)&readBuf[2];
                dataSize = ((QT3DSU32)readBuf.size() / sizeof(unsigned short)) - 1;
                qt3ds::foundation::ConvertUTF(dataPtr, dataSize, convertBuf);
            } else if (endianIndicator == (QT3DSU16)0xFFFE || readBuf[0] == 0) {
                QT3DS_ASSERT(false);
            } else {
                convertBuf.resize(readBuf.size());
                const char8_t *inPtr = (const char8_t *)readBuf.data();
                const char8_t *endPtr = inPtr + readBuf.size();
                convertBuf.assign(inPtr, endPtr);
            }

            return ParseLuaData(inFactory, inStringTable, convertBuf, inErrorHandler);
        }
    };
}
}

#endif
