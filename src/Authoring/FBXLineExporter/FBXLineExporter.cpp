/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include <qglobal.h>
#include "qtAuthoring-config.h"
#ifdef QT_3DSTUDIO_FBX
#include <fbxsdk.h>
#include "eastl/string.h"
#include "foundation/FileTools.h"
#include "IOStreams.h"

using namespace eastl;
using namespace qt3ds;
using namespace qt3ds::foundation;

namespace {
struct SLineExporter
{
    FbxManager *m_FbxManager;
    FbxScene *m_FbxScene;
    IOutStream *m_OutStream;
    SLineExporter()
        : m_FbxManager(FbxManager::Create())
        , m_FbxScene(NULL)
        , m_OutStream(NULL)
    {
        FbxIOSettings *theIOSettings = FbxIOSettings::Create(m_FbxManager, IOSROOT);
        m_FbxManager->SetIOSettings(theIOSettings);
        m_FbxScene = FbxScene::Create(m_FbxManager, "");
    }

    bool OpenFile(const char *inFilePath)
    {
        FbxImporter *theImporter = FbxImporter::Create(m_FbxManager, "");
        if (!theImporter->Initialize(inFilePath, -1, m_FbxManager->GetIOSettings())) {
            theImporter->Destroy();
            return false;
        }
        int major = 0, minor = 0, revision = 0;
        theImporter->GetFileVersion(major, minor, revision);

        if (theImporter->IsFBX()) {
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_GOBO, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_ANIMATION, true);
            m_FbxManager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
        }
        bool theStatus = theImporter->Import(m_FbxScene);
        theImporter->Destroy();
        return theStatus;
    }

    ~SLineExporter()
    {
        if (m_FbxScene != NULL)
            m_FbxScene->Destroy();
        if (m_FbxManager != NULL)
            m_FbxManager->Destroy();
    }

    void Write(const char *data)
    {
        if (isTrivial(data))
            return;
        m_OutStream->Write(data, (QT3DSU32)strlen(data));
    }

    void ProcessNodeChildren(FbxNode *inFbxNode)
    {

        for (int i = 0, end = inFbxNode->GetChildCount(); i < end; ++i) {
            ProcessNode(inFbxNode->GetChild(i));
        }
    }

    static bool AreEqual(QT3DSF64 lhs, QT3DSF64 rhs) { return abs((long)lhs - (long)rhs) < .0001; }

    void ProcessNode(FbxNode *inNode)
    {
        if (inNode == NULL)
            return;
        if (inNode->GetNodeAttribute() != NULL
            && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLine
            && !isTrivial(inNode->GetName())) {
            const char *nodeName = inNode->GetName();
            FbxLine *theLine = inNode->GetLine();
            (void)theLine;
            // Only support translating the point set, not anything else.
            FbxDouble3 theTranslation = inNode->LclTranslation.Get();
            fprintf(stdout, "Exporting line %s\n", nodeName);
            Write("local ");
            Write(nodeName);
            Write("Data = {\n");
            char buffer[1024];
            FbxDouble2 theLastPoint;
            for (int idx = 0, end = theLine->GetIndexArraySize(); idx < end; ++idx) {
                int pointIndex = theLine->GetPointIndexAt(idx);
                FbxDouble4 thePoint = theLine->GetControlPointAt(pointIndex);
                thePoint[0] += theTranslation[0];
                thePoint[1] += theTranslation[1];
                bool writeData = true;
                if (idx) {
                    writeData = !(AreEqual(thePoint[0], theLastPoint[0])
                                  && AreEqual(thePoint[1], theLastPoint[1]));
                }

                // Do a simple de-dup; identical points in the list present problems.
                if (writeData) {
                    sprintf(buffer, "\t{%g, %g},\n", thePoint[0], thePoint[1]);
                    Write(buffer);
                }
                theLastPoint[0] = thePoint[0];
                theLastPoint[1] = thePoint[1];
            }
            Write("}\n");

            Write("allLines[\"");
            Write(nodeName);
            Write("\"] = ");
            Write(nodeName);
            Write("Data\n\n");
        }
        ProcessNodeChildren(inNode);
    }

    void ExportLines(IOutStream &outStream)
    {
        m_OutStream = &outStream;
        Write("local allLines = {}\n\n");
        ProcessNode(m_FbxScene->GetRootNode());
        Write("\n\nreturn allLines\n");
    }
};
}

int main(int c, char **v)
{
    if (c == 1) {
        fprintf(stderr, "No input file given\n");
        return 1;
    }
    string fullPath(v[1]);
    if (fullPath.find('\"') == 0) {
        fullPath.resize(fullPath.size() - 1);
        fullPath.erase(fullPath.begin(), fullPath.begin() + 1);
    }

    SLineExporter theExporter;
    if (!theExporter.OpenFile(fullPath.c_str())) {
        fprintf(stderr, "Failed to open input file: \"%s\"\n", fullPath.c_str());
    }

    string dirname, filename, extension;
    CFileTools::Split(fullPath.c_str(), dirname, filename, extension);
    string outputFileName(filename);
    outputFileName.append(".lua");
    string outputFilePath;
    CFileTools::CombineBaseAndRelative(dirname.c_str(), outputFileName.c_str(), outputFilePath);
    CFileSeekableIOStream theOutputStream(outputFilePath.c_str(), FileWriteFlags());
    if (theOutputStream.IsOpen() == false) {
        fprintf(stderr, "Failed to open output file: \"%s\"\n", outputFilePath.c_str());
        return 1;
    }
    theExporter.ExportLines(theOutputStream);
}
#else
int main(int, char **)
{
    qCritical("FBX SDK not available!");
    return 1;
}
#endif
