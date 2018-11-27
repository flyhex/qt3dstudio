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

#include "PresentationFile.h"
#include "ProjectFile.h"
#include "Dialogs.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "StudioUtils.h"
#include "IDocumentReader.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include <QtCore/qfile.h>
#include <QtCore/qsavefile.h>
#include <QtXml/qdom.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qlist.h>

// This class provides utility static methods for working with presentation files (.uip). Old uip
// functionality should be gradually moved here whenever feasible.

// static
QSize PresentationFile::readSize(const QString &uipPath)
{
    QFile file(uipPath);
    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return QSize();
    }

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("ProjectSettings")) {
            const auto attrs = reader.attributes();
            return QSize(attrs.value(QLatin1String("presentationWidth")).toInt(),
                         attrs.value(QLatin1String("presentationHeight")).toInt());
        }
    }

    return QSize();
}

/**
 * Find all occurrences of a presentation Id in a .uip file and replace them with a new value
 *
 * @param uipPath presentation file path
 * @param oldId the presentation Id to find
 * @param newId the presentation Id to replace
 */
// static
void PresentationFile::updatePresentationId(const QString &uipPath, const QString &oldId,
                                            const QString &newId)
{
    QDomDocument domDoc;
    QSaveFile file(uipPath);
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomElement rootElem = domDoc.documentElement();
    QDomNodeList addNodes = rootElem.elementsByTagName(QStringLiteral("Add"));
    bool updated = false;

    if (!addNodes.isEmpty()) {
        for (int i = 0; i < addNodes.length(); ++i) {
            QDomElement elem = addNodes.at(i).toElement();
            if (elem.attribute(QStringLiteral("sourcepath")) == oldId) {
                elem.setAttribute(QStringLiteral("sourcepath"), newId);
                updated = true;
            }
            if (elem.attribute(QStringLiteral("subpresentation")) == oldId) {
                elem.setAttribute(QStringLiteral("subpresentation"), newId);
                updated = true;
            }
        }
    }

    if (updated)
        StudioUtils::commitDomDocumentSave(file, domDoc);
}

/**
 * Find all occurrences of a material name in a .uip file and replace them with a new value
 *
 * @param uipPath presentation file path
 * @param oldName the material name to find
 * @param newName the material name to replace
 */
// static
void PresentationFile::renameMaterial(const QString &uipPath, const QString &oldName,
                                      const QString &newName)
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    const auto sceneEditor = doc->getSceneEditor();

    const auto absOldPath = sceneEditor->getFilePathFromMaterialName(oldName);
    const auto absNewPath = sceneEditor->getFilePathFromMaterialName(newName);

    const auto dir = QFileInfo(uipPath).dir();
    const auto relOldPath = dir.relativeFilePath(absOldPath);
    const auto relNewPath = dir.relativeFilePath(absNewPath);

    auto refNewName = newName;
    int slashIndex = newName.lastIndexOf(QLatin1Char('/'));
    if (slashIndex != -1)
        refNewName = newName.mid(slashIndex + 1);

    QDomDocument domDoc;
    QSaveFile file(uipPath);
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomElement rootElem = domDoc.documentElement();
    QDomNodeList addNodes = rootElem.elementsByTagName(QStringLiteral("Add"));
    QDomNodeList setNodes = rootElem.elementsByTagName(QStringLiteral("Set"));

    bool updated = false;

    QDomNodeList materialNodes = rootElem.elementsByTagName(QStringLiteral("Material"));

    // Find the material container
    QDomElement materialContainer;
    for (int i = 0; i < materialNodes.length(); ++i) {
        QDomElement elem = materialNodes.at(i).toElement();
        if (elem.attribute(QStringLiteral("id")) == bridge->getMaterialContainerName()) {
            materialContainer = elem;
            break;
        }
    }

    if (materialContainer.isNull())
        return;

    QDomNodeList containerNodes = materialContainer.childNodes();

    // Store the material ids for further use and change the names and ids to new ones
    QStringList materialIds;
    for (int i = 0; i < containerNodes.length(); ++i) {
        QDomElement elem = containerNodes.at(i).toElement();
        materialIds.append(elem.attribute(QStringLiteral("id")));
        if (elem.attribute(QStringLiteral("id")) == oldName) {
            elem.setAttribute(QStringLiteral("id"), newName);
            updated = true;
        }
    }

    // Since rename can change the visible name only in the original presentation
    // and retain the old id, we have to cross-reference the ids here to the ids
    // logged previously. If a material has the same visible name as the old name,
    // the id and the new name are stored
    QVector<QPair<QString, QString>> materialRenames;

    const auto renameReferencedMaterial = [&](QDomElement &elem) {
        QString ref = elem.attribute(QStringLiteral("ref"));
        if (elem.attribute(QStringLiteral("name")) == oldName
                && !ref.isEmpty() && materialIds.contains(ref.mid(1))) {
            materialRenames.append(QPair<QString, QString>(ref.mid(1), newName));
            elem.setAttribute(QStringLiteral("ref"), QLatin1Char('#') + newName);
            elem.setAttribute(QStringLiteral("name"), newName);
            updated = true;
        }
        if (elem.attribute(QStringLiteral("sourcepath")) == relOldPath) {
            elem.setAttribute(QStringLiteral("sourcepath"), relNewPath);
            if (elem.hasAttribute(QStringLiteral("referencedmaterial"))) {
                elem.setAttribute(QStringLiteral("referencedmaterial"),
                                  QLatin1Char('#') + newName);
                elem.setAttribute(QStringLiteral("name"), refNewName);
            }
            updated = true;
        }
    };

    for (int i = 0; i < addNodes.length(); ++i) {
        QDomElement elem = addNodes.at(i).toElement();
        renameReferencedMaterial(elem);
    }

    for (int i = 0; i < setNodes.length(); ++i) {
        QDomElement elem = setNodes.at(i).toElement();
        renameReferencedMaterial(elem);
    }

    // New pass is needed to change the ids stored when changing the Add and Set Nodes
    for (int i = 0; i < containerNodes.length(); ++i) {
        for (auto &materialRename : qAsConst(materialRenames)) {
            QDomElement elem = containerNodes.at(i).toElement();
            if (elem.attribute(QStringLiteral("id")) == materialRename.first) {
                elem.setAttribute(QStringLiteral("id"), materialRename.second);
                updated = true;
            }
        }
    }

    if (updated)
        StudioUtils::commitDomDocumentSave(file, domDoc);
}

/**
 * Find the project file path matching the given presentation file path
 *
 * @param uipPath presentation file path
 * @return project file absolute path
 */
// static
QString PresentationFile::findProjectFile(const QString &uipPath)
{
    QFileInfo fi(uipPath);

    // first check if there is a uia in the same folder as the uip with the same name
    QString uiaPath = fi.dir().absoluteFilePath(fi.completeBaseName() + QStringLiteral(".uia"));
    if (QFile::exists(uiaPath))
        return uiaPath;

    // next search for a uia starting from uip folder and going up, the first found 1 is assumed
    // to be the project file
    QDir currDir = fi.dir();
    const int MAX_SEARCH_DEPTH = 3; // scan up to 3 levels up (for performance reasons)
    int searchDepth = 0;
    do {
        QDirIterator di(currDir.path(), QDir::NoDotAndDotDot | QDir::Files);
        while (di.hasNext()) {
            QFileInfo fi2 = di.next();
            if (fi2.suffix() == QLatin1String("uia"))
                return fi2.filePath();
        }
        ++searchDepth;
    } while (searchDepth < MAX_SEARCH_DEPTH && currDir.cdUp());

    return {};
}

// Get all available child assets source paths (materials, images, effects, etc).
// The source paths returned are relative to the presentation file being parsed.
// static
void PresentationFile::getSourcePaths(const QFileInfo &uipSrc, const QFileInfo &uipTarget,
                                      QHash<QString, QString> &outPathMap,
                                      QString &outProjPathSrc,
                                      QHash<QString, QString> &outPresentationNodes)
{
    QDomDocument domDoc;
    if (!StudioUtils::readFileToDomDocument(uipTarget.filePath(), domDoc))
        return;

    QVector<SubPresentationRecord> subpresentations;
    QString uiaPath = findProjectFile(uipSrc.filePath());
    if (!uiaPath.isEmpty()) {
        outProjPathSrc = QFileInfo(uiaPath).path();
        QString uipPathRelative = QFileInfo(uiaPath).dir().relativeFilePath(uipSrc.filePath());
        ProjectFile::getPresentations(uiaPath, subpresentations, uipPathRelative);
    } else {
        outProjPathSrc = uipSrc.path();
    }
    QDir srcProjDir(outProjPathSrc);
    QDir srcUipDir(uipSrc.path());

    const auto convertPath = [&](const QString &path, bool forceProj = false) -> QString {
        if (forceProj || path.startsWith(QLatin1String("./")))
            return srcUipDir.relativeFilePath(srcProjDir.absoluteFilePath(path));
        else
            return path; // Assuming path is already presentation relative
    };

    // Map to cache effect and material properties during the presentation file parsing,
    // as we don't yet know which ones are actually assets.
    // Key: Material/effect class id
    // Value: Set of material/effect properties
    QHash<QString, QSet<QString>> matEffPropertyMap;

    // search <Classes>
    QDomElement classesElem = domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Classes"));
    for (QDomElement p = classesElem.firstChild().toElement(); !p.isNull();
        p = p.nextSibling().toElement()) {
        const QString sourcepath = convertPath(p.attribute(QStringLiteral("sourcepath")));
        if (!sourcepath.isEmpty() && !outPathMap.contains(sourcepath)) {
            outPathMap.insert(sourcepath, {});

            QFileInfo fi(sourcepath);
            QByteArray ext = fi.suffix().toLatin1();

            // if material or effect, find and add their dependents
            if (CDialogs::IsMaterialFileExtension(ext.data()) ||
                CDialogs::IsEffectFileExtension(ext.data())) {
                QSet<QString> propertySet;
                QHash<QString, QString> matEffPathMap;
                g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                    .ParseSourcePathsOutOfEffectFile(
                            uipSrc.path() + QStringLiteral("/") + sourcepath,
                            outProjPathSrc, true, matEffPathMap, propertySet);
                // ParseSourcePathsOutOfEffectFile returns paths relative to project
                QHashIterator<QString, QString> pathIter(matEffPathMap);
                while (pathIter.hasNext()) {
                    pathIter.next();
                    outPathMap.insert(convertPath(pathIter.key(), true), pathIter.value());
                }
                matEffPropertyMap.insert(QStringLiteral("#") + p.attribute(QStringLiteral("id")),
                                         propertySet);
            }
        }
    }

    // mesh files for group imports, materials, and effects are found under <Graph>
    QDomElement graphElement = domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Graph"));

    QDomNodeList modelElems = graphElement.elementsByTagName(QStringLiteral("Model"));
    for (int i = 0; i < modelElems.count(); ++i) {
        QDomElement elem = modelElems.at(i).toElement();
        const QString sourcePath = convertPath(elem.attribute(QStringLiteral("sourcepath")));
        if (!sourcePath.isEmpty()) {
            QFileInfo fi(sourcePath);
            QByteArray ext = fi.suffix().toLatin1();
            if (CDialogs::isMeshFileExtension(ext.data())) {
                if (!outPathMap.contains(sourcePath))
                    outPathMap.insert(sourcePath, {});
                continue;
            }
        }
    }

    // find material and effect files instance ids
    QHash<QString, QString> matEffClassIdMap;
    auto parseMatEffIds = [&matEffClassIdMap](const QDomNodeList& nodes) {
        for (int i = 0; i < nodes.count(); ++i) {
            QDomElement elem = nodes.at(i).toElement();
            const QString classId = elem.attribute(QStringLiteral("class"));
            if (!classId.isEmpty()) {
                const QString id = elem.attribute(QStringLiteral("id"));
                matEffClassIdMap.insert(QStringLiteral("#") + id, classId);
            }
        }
    };

    parseMatEffIds(graphElement.elementsByTagName(QStringLiteral("Effect")));
    parseMatEffIds(graphElement.elementsByTagName(QStringLiteral("CustomMaterial")));

    // search <Logic> -> <State> -> <Add>
    QDomNodeList addElems = domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Logic"))
            .elementsByTagName(QStringLiteral("Add"));

    for (int i = 0; i < addElems.count(); ++i) {
        QDomElement elem = addElems.at(i).toElement();
        const QString sourcePath = convertPath(elem.attribute(QStringLiteral("sourcepath")));
        if (!sourcePath.isEmpty()) {
            QFileInfo fi(sourcePath);
            QByteArray ext = fi.suffix().toLatin1();
            // supported types:
            // images, custom mesh files for basic objects, import files, materialdef files
            if (CDialogs::IsImageFileExtension(ext.data())
                    || CDialogs::isMeshFileExtension(ext.data())
                    || CDialogs::isImportFileExtension(ext.data())
                    || CDialogs::IsMaterialFileExtension(ext.data())) {
                if (!outPathMap.contains(sourcePath))
                    outPathMap.insert(sourcePath, {});
            } else {
                // add layer subpresentations paths
                auto *sp = std::find_if(subpresentations.begin(), subpresentations.end(),
                                       [&sourcePath](const SubPresentationRecord &spr) -> bool {
                                           return spr.m_id == sourcePath;
                                       });
                if (sp != subpresentations.end()) { // has a subpresentation
                    QString spPath = convertPath(sp->m_argsOrSrc, true);
                    if (!outPathMap.contains(spPath)) {
                        outPathMap.insert(spPath, {});
                        outPresentationNodes.insert(spPath, sp->m_id);
                    }
                }
            }
        }

        // add texture subpresentations paths
        QString subpresentation = elem.attribute(QStringLiteral("subpresentation"));
        if (!subpresentation.isEmpty()) {
            auto *sp = std::find_if(subpresentations.begin(), subpresentations.end(),
                                   [&subpresentation](const SubPresentationRecord &spr) -> bool {
                                       return spr.m_id == subpresentation;
                                   });
            if (sp != subpresentations.end()) { // has a subpresentation
                QString spPath = convertPath(sp->m_argsOrSrc, true);
                if (!outPathMap.contains(spPath)) {
                    outPathMap.insert(spPath, {});
                    outPresentationNodes.insert(spPath, sp->m_id);
                }
            }
        }

        // add fonts paths
        QString font = elem.attribute(QStringLiteral("font"));
        if (!font.isEmpty()) {
            // the .uip file only shows the font name, we search for the font file in the current
            // directory plus the 'fonts' directory at the same level or 1 level up.

            const QString TTF_EXT = QStringLiteral(".ttf"); // TODO: should we also consider .otf?
            const QString slashUipPath = uipSrc.path() + QLatin1Char('/');

            // this is the most probable place so lets search it first
            QString fontPath = QStringLiteral("../fonts/") + font + TTF_EXT;
            QFileInfo absFontPath(slashUipPath + fontPath);
            if (absFontPath.exists()) {
                if (!outPathMap.contains(fontPath))
                    outPathMap.insert(fontPath, absFontPath.absoluteFilePath());
            } else {
                fontPath = font + TTF_EXT;
                absFontPath = QFileInfo(slashUipPath + fontPath);
                if (absFontPath.exists()) {
                    if (!outPathMap.contains(fontPath))
                        outPathMap.insert(fontPath, absFontPath.absoluteFilePath());
                } else {
                    fontPath = QStringLiteral("fonts/") + font + TTF_EXT;
                    absFontPath = QFileInfo(slashUipPath + fontPath);
                    if (absFontPath.exists()) {
                        if (!outPathMap.contains(fontPath))
                            outPathMap.insert(fontPath, absFontPath.absoluteFilePath());
                    }
                }
            }
        }

        // add custom material/effect assets
        const QString ref = elem.attribute(QStringLiteral("ref"));
        const QString classId = matEffClassIdMap.value(ref, {});
        if (!classId.isEmpty()) {
            const QSet<QString> textureProps = matEffPropertyMap.value(classId, {});
            for (auto &prop : textureProps) {
                QString texturePath = elem.attribute(prop);
                if (!texturePath.isEmpty()) {
                    // Typically these paths have ./ prepended even though they are relative to uip
                    // Remove it as ./ at start is interpreted as relative to project file
                    if (texturePath.startsWith(QLatin1String("./")))
                        texturePath = texturePath.mid(2);
                    if (!texturePath.isEmpty()) {
                        QFileInfo absTexPath(uipSrc.path() + QLatin1Char('/') + texturePath);
                        if (!outPathMap.contains(texturePath) && absTexPath.exists()
                                && absTexPath.isFile()) {
                            outPathMap.insert(texturePath, absTexPath.absoluteFilePath());
                        }
                    }
                }
            }
        }
    }
}

/**
 * Find datainput use in subpresentation
 *
 * @param subpresentation subpresentation
 * @param outmap returned list of datainput - property name pairs
 */
// static
bool PresentationFile::getDataInputBindings(const SubPresentationRecord &subpresentation,
                                            QMultiMap<QString, QPair<QString, QString>> &outmap)
{
    QList<QString> ctrldPropList;

    QString spPath(g_StudioApp.getRenderableAbsolutePath(subpresentation.m_id));

    QDomDocument domDoc;
    if (!StudioUtils::readFileToDomDocument(spPath, domDoc))
        return false;

    // search <Graph>
    QDomElement graphElem = domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Graph"));
    for (QDomElement p = graphElem.firstChild().toElement(); !p.isNull();
         p = p.nextSibling().toElement()) {
        QString ctrldPropStr = p.attribute(QStringLiteral("controlledproperty"));
        if (!ctrldPropStr.isEmpty())
            ctrldPropList.append(ctrldPropStr);
    }
    // Search Logic - State - Add
    QDomNodeList addElems = domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Logic"))
            .elementsByTagName(QStringLiteral("Add"));

    for (int i = 0; i < addElems.count(); ++i) {
        QDomElement elem = addElems.at(i).toElement();
        QString ctrldPropStr = elem.attribute(QStringLiteral("controlledproperty"));
        if (!ctrldPropStr.isEmpty())
            ctrldPropList.append(ctrldPropStr);
    }

    for (auto di : qAsConst(ctrldPropList)) {
        QStringList split = di.split(QLatin1String(" "));
        for (int i = 0; i < split.size(); i += 2) {
            QString diName = split[i];
            // Datainput names indicated with prefix "$", remove
            // if found.
            if (diName.startsWith(QLatin1String("$")))
                diName = diName.mid(1, diName.size() - 1);
            QString propName = split[i + 1];
            // We should find the datainput from the global datainput list
            // parsed out from UIA file, but check just in case and do not insert
            // if not found.
            if (g_StudioApp.m_dataInputDialogItems.contains(diName)) {
                outmap.insert(subpresentation.m_id, QPair<QString, QString>(diName, propName));
            } else {
                qWarning() << "Subpresentation" << subpresentation.m_id
                           << "is using datainput" << diName << "that is "
                              "not found from the current UIA file";
            }
        }
    }
    return true;
}
