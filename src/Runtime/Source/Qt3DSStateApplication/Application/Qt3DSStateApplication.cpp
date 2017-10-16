/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#include "Qt3DSStateApplication.h"
#include "foundation/FileTools.h"
#include "EASTL/vector.h"
#include "foundation/IOStreams.h"
#include "foundation/XML.h"

#include <QtCore/qstringlist.h>

typedef eastl::string TAppStr;

using namespace qt3ds::foundation;
using namespace qt3ds::state;
using namespace eastl;

bool IApplication::EnsureApplicationFile(const char *inFullUIPPath,
                                         const QStringList &presentations)
{
    eastl::string directory;
    eastl::string filestem;
    eastl::string extension;
    eastl::string filename;

    CFileTools::Split(inFullUIPPath, directory, filestem, extension);
    filename = filestem;
    filename.append(1, '.');
    filename.append(extension);
    CFileTools::CreateDir(directory.c_str());
    eastl::vector<eastl::string> dirFiles;
    CFileTools::GetDirectoryEntries(directory, dirFiles);
    eastl::string uiaPath;

    for (qt3ds::QT3DSU32 idx = 0, end = dirFiles.size(); idx < end && uiaPath.empty(); ++idx) {
        eastl::string fileExt;
        CFileTools::GetExtension(dirFiles[idx].c_str(), fileExt);
        if (fileExt.comparei("uia") == 0)
            CFileTools::CombineBaseAndRelative(directory.c_str(), dirFiles[idx].c_str(), uiaPath);
    }

    if (uiaPath.size()) {
        MallocAllocator allocator;
        NVScopedRefCounted<IStringTable> strTable(IStringTable::CreateStringTable(allocator));
        NVScopedRefCounted<IDOMFactory> domFactory(
            IDOMFactory::CreateDOMFactory(allocator, strTable));
        eastl::pair<SNamespacePairNode *, SDOMElement *> readResult;
        {
            CFileSeekableIOStream theInStream(uiaPath.c_str(), FileReadFlags());
            readResult = CDOMSerializer::Read(*domFactory, theInStream);
        }
        if (readResult.second == NULL) {
            QT3DS_ASSERT(false);
            uiaPath.clear();
        } else {
            eastl::pair<NVScopedRefCounted<IDOMWriter>, NVScopedRefCounted<IDOMReader>> writerData
                = IDOMWriter::CreateDOMWriter(domFactory, *readResult.second, strTable);
            NVScopedRefCounted<IDOMReader> domReader(writerData.second);
            if (!domReader->MoveToFirstChild("assets"))
                writerData.first->Begin("assets");
            else {
                int pre = domReader->CountChildren("presentation");
                int preq = domReader->CountChildren("presentation-qml");
                if (pre + preq > 0) {
                    while (domReader->MoveToFirstChild("presentation"))
                        writerData.first->RemoveCurrent();
                    while (domReader->MoveToFirstChild("presentation-qml"))
                        writerData.first->RemoveCurrent();
                }
            }

            writerData.first->Begin("presentation");
            writerData.first->Att("id", filestem.c_str());
            writerData.first->Att("src", filename.c_str());
            writerData.first->End();

            for (int i = 0; i < presentations.size() / 3; i++) {
                writerData.first->Begin(qPrintable(presentations[3 * i]));
                writerData.first->Att("id", qPrintable(presentations[3 * i + 1]));
                if (presentations[3 * i] == QStringLiteral("presentation"))
                    writerData.first->Att("src", qPrintable(presentations[3 * i + 2]));
                else
                    writerData.first->Att("args", qPrintable(presentations[3 * i + 2]));
                writerData.first->End();
            }

            CFileSeekableIOStream theOutStream(uiaPath.c_str(), FileWriteFlags());
            if (theOutStream.IsOpen()) {
                NVDataRef<SNamespacePair> namespacePairs;
                if (readResult.first)
                    namespacePairs = toDataRef<SNamespacePair>(*readResult.first);
                CDOMSerializer::WriteXMLHeader(theOutStream);
                CDOMSerializer::Write(allocator, *readResult.second, theOutStream, *strTable,
                                      namespacePairs, false);
            } else
                return false;
        }
    }
    // No uia, just create a new one named after the uip file
    if (!uiaPath.size()) {
        eastl::string uiaName = filestem + ".uia";
        CFileTools::CombineBaseAndRelative(directory.c_str(), uiaName.c_str(), uiaPath);
        eastl::string uiaFileData = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                    "<application xmlns=\"http://qt.io/qt3dstudio/uia\">\n"
                                    "\t<assets initial=\"";
        uiaFileData.append(filestem.c_str());
        uiaFileData.append("\">\n"
                           "\t\t<presentation id=\"");
        uiaFileData.append(filestem.c_str());
        uiaFileData.append("\"\tsrc=\"");
        uiaFileData.append(filename);
        uiaFileData.append("\"/>\n");

        for (int i = 0; i < presentations.size() / 3; i++) {
            uiaFileData.append("\t\t<");
            uiaFileData.append(qPrintable(presentations[3 * i]));
            uiaFileData.append("id=\"");
            uiaFileData.append(qPrintable(presentations[3 * i + 1]));
            if (presentations[3 * i] == QStringLiteral("presentation"))
                uiaFileData.append("\" src=\"");
            else
                uiaFileData.append("\" args=\"");
            uiaFileData.append(qPrintable(presentations[3 * i + 2]));
            uiaFileData.append("\"/>\n");
        }

        uiaFileData.append("\t</assets>\n");

        // Add initial state
        uiaFileData.append("\n\t<statemachine ref=\"#logic\">\n");
        uiaFileData.append("\t\t<visual-states>\n");
        uiaFileData.append("\t\t\t<state ref=\"Initial\">\n");
        uiaFileData.append("\t\t\t\t<enter>\n");
        uiaFileData.append("\t\t\t\t\t<goto-slide element=\"main:Scene\" rel=\"next\"/>\n");
        uiaFileData.append("\t\t\t\t</enter>\n");
        uiaFileData.append("\t\t\t</state>\n");
        uiaFileData.append("\t\t</visual-states>\n");
        uiaFileData.append("\t</statemachine>\n");

        uiaFileData.append("</application>\n");

        CFileSeekableIOStream theStream(uiaPath.c_str(), FileWriteFlags());
        if (!theStream.IsOpen()) {
            QT3DS_ASSERT(false);
            return false;
        }
        theStream.Write(
            toConstDataRef((const qt3ds::QT3DSU8 *)uiaFileData.c_str(),
                           (qt3ds::QT3DSU32)uiaFileData.length()));
    }
    return true;
}

eastl::string IApplication::GetLaunchFile(const char *inFullUIPPath)
{
    eastl::string directory;
    eastl::string filestem;
    eastl::string extension;
    eastl::string filename;

    CFileTools::Split(inFullUIPPath, directory, filestem, extension);
    eastl::string uiaPath;

    eastl::vector<eastl::string> dirFiles;
    CFileTools::GetDirectoryEntries(directory, dirFiles);

    for (qt3ds::QT3DSU32 idx = 0, end = dirFiles.size(); idx < end && uiaPath.empty(); ++idx) {
        eastl::string fileExt;
        CFileTools::GetExtension(dirFiles[idx].c_str(), fileExt);
        if (fileExt.comparei("uia") == 0)
            CFileTools::CombineBaseAndRelative(directory.c_str(), dirFiles[idx].c_str(), uiaPath);
    }
    return uiaPath.empty() == false ? uiaPath : eastl::string(inFullUIPPath);
}
