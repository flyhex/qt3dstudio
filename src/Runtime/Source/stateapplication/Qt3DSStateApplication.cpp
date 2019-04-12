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

eastl::string IApplication::GetLaunchFile(const char *inFullUIPPath)
{
    eastl::string directory;
    eastl::string filestem;
    eastl::string extension;

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
