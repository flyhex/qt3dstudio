/****************************************************************************
**
** Copyright (C) 1999-2011 NVIDIA Corporation.
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

#ifndef UICIMPORTTRANSLATIONH
#define UICIMPORTTRANSLATIONH

#include "qtAuthoring-config.h"
#include "UICImportPerformImport.h"

namespace UICIMP {

class Import;

typedef enum _ESceneGraphWarningCode {
    ESceneGraphWarningCode_OnlySupportTriangles, ///< Model contains geometric elements other than
                                                 ///triangles (note that this won't throw exception,
                                                 ///just pop up warning)
    ESceneGraphWarningCode_TrianglesDuplicateSemantic, ///< Triangle contains duplicate semantics,
                                                       ///ex: 1 triangle has multiple TEXCOORD
                                                       ///(multiple UV maps)
    ESceneGraphWarningCode_VertexBufferTooLarge, ///< Triangle contains duplicate semantics, ex: 1
                                                 ///triangle has multiple TEXCOORD (multiple UV
                                                 ///maps)
    ESceneGraphWarningCode_MissingSourceFile, ///< Couldn't find a source image file
    ESceneGraphWarningCode_LockedDestFile, ///< An image or mesh file we need to write to is not
                                           ///writeable.
    ESceneGraphWarningCode_Generic, ///< For generic warnings
} ESceneGraphWarningCode;

class ISGTranslationLog
{
protected:
    virtual ~ISGTranslationLog() {}
public:
    virtual void OnWarning(ESceneGraphWarningCode inWarningCode,
                           const wchar_t *inAssociatedName) = 0;
};

#ifdef UICIMPORT_TRANSLATION_INTERNAL
#define EXPORT_FUNCTION
#else
#define EXPORT_FUNCTION
#endif

class CImportTranslation
{
public:
    static inline const wchar_t *GetRootNodeId() { return L"__import__root__"; }
    // Parse the collada file, marking when things don't line up correctly.
    // Translate a collada file into an import datastructure.
    static EXPORT_FUNCTION bool ParseColladaFile(const std::string &fileName, Import &import,
                                                 ISGTranslationLog &log);
#ifdef QT_3DSTUDIO_FBX
    // Parse the FBX file.
    // Translate a FBX file into an import datastructure.
    static EXPORT_FUNCTION bool ParseFbxFile(const std::string &fileName, Import &import,
                                             ISGTranslationLog &log);
#endif
};

struct STranslationLog : public UICIMP::ISGTranslationLog
{
    vector<pair<UICIMP::ESceneGraphWarningCode, Q3DStudio::CString>> m_Warnings;
    void OnWarning(UICIMP::ESceneGraphWarningCode inWarningCode,
                           const wchar_t *inAssociatedName) override
    {
        m_Warnings.push_back(std::make_pair(inWarningCode, inAssociatedName));
    }
};

struct SColladaTranslator : public UICIMP::ITranslator
{
    QString m_SourceFile;
    STranslationLog m_TranslationLog;

    SColladaTranslator(const QString& srcFile)
        : m_SourceFile(srcFile)
    {
    }

    // Object is created on the stack, no reason to release.
    void Release() override {}

    const QString &GetSourceFile() override { return m_SourceFile; }
    // Returning false causes the rest of the import or refresh process
    // to fail.
    bool PerformTranslation(Import &import) override
    {
        return CImportTranslation::ParseColladaFile(m_SourceFile.toStdString(), import,
                                                    m_TranslationLog);
    }
};

#ifdef QT_3DSTUDIO_FBX
struct SFbxTranslator : public UICIMP::ITranslator
{
    QString m_SourceFile;
    STranslationLog m_TranslationLog;

    SFbxTranslator(const QString& srcFile)
        : m_SourceFile(srcFile)
    {
    }

    // Object is created on the stack, no reason to release.
    void Release() override {}

    const QString &GetSourceFile() override { return m_SourceFile; }
    // Returning false causes the rest of the import or refresh process
    // to fail.
    bool PerformTranslation(Import &import) override
    {
        return CImportTranslation::ParseFbxFile(m_SourceFile.toStdString(), import,
                                                m_TranslationLog);
    }
};
#endif
}
#endif
