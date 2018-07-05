/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef PROJECTFILE_H
#define PROJECTFILE_H

#include "Qt3DSFileTools.h"

namespace Q3DStudio {
class CFilePath;
class CString;
}
struct SubPresentationRecord;
class CDataInputDialogItem;

class ProjectFile
{
public:
    ProjectFile();

    void create(const Q3DStudio::CString &projectName,
                const Q3DStudio::CFilePath &projectPath);
    void setProjectNameAndPath(const Q3DStudio::CString &projectName,
                               const Q3DStudio::CFilePath &projectPath);
    void ensureProjectFile(const QDir &uipDirectory);
    void loadSubpresentationsAndDatainputs(
            QVector<SubPresentationRecord> &subpresentations,
            QMap<QString, CDataInputDialogItem *> &datainputs);
    void writePresentationId(const QString &id, const QString &src = QString());
    void updateDocPresentationId();
    bool isUniquePresentationId(const QString &id,
                                const QString &src = QString()) const;
    Q3DStudio::CFilePath getProjectPath() const;
    Q3DStudio::CString getProjectName() const;
    QString getFirstPresentationPath(const QString &uiaPath) const;
    QString getPresentationId(const QString &src) const;
    void addPresentationNode(const Q3DStudio::CFilePath &uip);

private:
    QString ensureUniquePresentationId(const QString &id) const;

    Q3DStudio::CFilePath m_projectPath; // project directory
    Q3DStudio::CString m_projectName;
};

#endif // PROJECTFILE_H
