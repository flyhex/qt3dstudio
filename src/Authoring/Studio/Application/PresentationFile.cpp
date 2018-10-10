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
#include "IDocumentReader.h"
#include <QtCore/qfile.h>
#include <QtXml/qdom.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>
#include <qxmlstream.h>
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
    QFile file(uipPath);
    if (!file.open(QFile::Text | QIODevice::ReadWrite)) {
        qWarning() << file.errorString();
        return;
    }
    QDomDocument domDoc;
    domDoc.setContent(&file);

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

    if (updated) {
        file.resize(0);
        file.write(domDoc.toByteArray(4));
    }
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

// get all available child assets source paths (materials, images, effects, etc)
// static
void PresentationFile::getSourcePaths(const QFileInfo &uipSrc, const QFileInfo &uipTarget,
                                      QHash<QString, QString> &outPathMap,
                                      QString &outProjPathSrc,
                                      QHash<QString, QString> &outPresentationNodes)
{
    QFile file(uipTarget.filePath());
    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return;
    }

    QDomDocument domDoc;
    domDoc.setContent(&file);

    QVector<SubPresentationRecord> subpresentations;
    QString uiaPath = findProjectFile(uipSrc.filePath());
    if (!uiaPath.isEmpty()) {
        outProjPathSrc = QFileInfo(uiaPath).path();
        QString uipPathRelative = QFileInfo(uiaPath).dir().relativeFilePath(uipSrc.filePath());
        ProjectFile::getPresentations(uiaPath, subpresentations, uipPathRelative);
    }

    // search <Classes>
    QDomElement classesElem = domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Classes"));
    for (QDomElement p = classesElem.firstChild().toElement(); !p.isNull();
        p = p.nextSibling().toElement()) {
        const QString sourcepath = p.attribute(QStringLiteral("sourcepath"));
        if (!sourcepath.isEmpty() && !outPathMap.contains(sourcepath)) {
            outPathMap.insert(sourcepath, {});

            QFileInfo fi(sourcepath);
            QByteArray ext = fi.suffix().toLatin1();

            // if material or effect, find and add their dependents
            if (CDialogs::IsMaterialFileExtension(ext.data()) ||
                CDialogs::IsEffectFileExtension(ext.data())) {
                g_StudioApp.GetCore()->GetDoc()->GetDocumentReader()
                    .ParseSourcePathsOutOfEffectFile(
                            uipSrc.path() + QStringLiteral("/") + sourcepath,
                            outPathMap, outProjPathSrc);
            }
        }
    }

    // search <Logic> -> <State> -> <Add>
    QDomNodeList addElems = domDoc.documentElement().firstChild()
            .firstChildElement(QStringLiteral("Logic"))
            .elementsByTagName(QStringLiteral("Add"));

    for (int i = 0; i < addElems.count(); ++i) {
        QDomElement elem = addElems.at(i).toElement();
        const QString sourcePath = elem.attribute(QStringLiteral("sourcepath"));
        if (!sourcePath.isEmpty()) {
            QFileInfo fi(sourcePath);
            QByteArray ext = fi.suffix().toLatin1();
            // add image and mesh paths
            if (CDialogs::IsImageFileExtension(ext.data())
                    || CDialogs::isMeshFileExtension(ext.data())) {
                if (!outPathMap.contains(sourcePath))
                    outPathMap.insert(sourcePath, {});
                continue;
            }

            // add layer subpresentations paths
            auto *sp = std::find_if(subpresentations.begin(), subpresentations.end(),
                                   [&sourcePath](const SubPresentationRecord &spr) -> bool {
                                       return spr.m_id == sourcePath;
                                   });
            if (sp != subpresentations.end()) { // has a subpresentation
                QString spPath = sp->m_argsOrSrc;
                // set as root path (if it is not root nor relative) to make sure importing works
                // correctly.
                if (!spPath.startsWith(QLatin1String("../"))
                    && !spPath.startsWith(QLatin1String("./"))) {
                    spPath.prepend(QLatin1String("./"));
                }

                if (!outPathMap.contains(spPath)) {
                    outPathMap.insert(spPath, {});
                    outPresentationNodes.insert(sp->m_argsOrSrc, sp->m_id);
                }
                continue;
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
                QString spPath = sp->m_argsOrSrc;
                // set as root path (if it is not root nor relative) to make sure importing works
                // correctly.
                if (!spPath.startsWith(QLatin1String("../"))
                    && !spPath.startsWith(QLatin1String("./"))) {
                    spPath.prepend(QLatin1String("./"));
                }

                if (!outPathMap.contains(spPath)) {
                    outPathMap.insert(spPath, {});
                    outPresentationNodes.insert(sp->m_argsOrSrc, sp->m_id);
                }
            }
            continue;
        }

        // add fonts paths
        QString font = elem.attribute(QStringLiteral("font"));
        if (!font.isEmpty()) {
            // the .uip file only shows the font name, we search for the font file in the current
            // directory plus the 'fonts' directory at the same level or 1 level up.

            const QString TTF_EXT = QStringLiteral(".ttf"); // TODO: should we also consider .otf?

            // this is the most probable place so lets search it first
            QString fontPath = QStringLiteral("../fonts/") + font + TTF_EXT;
            QString absFontPath = uipSrc.path() + QStringLiteral("/") + fontPath;
            if (QFile::exists(absFontPath)) {
                if (!outPathMap.contains(fontPath))
                    outPathMap.insert(fontPath, absFontPath);
                continue;
            }

            fontPath = font + TTF_EXT;
            absFontPath = uipSrc.path() + QStringLiteral("/") + fontPath;
            if (QFile::exists(absFontPath)) {
                if (!outPathMap.contains(fontPath))
                    outPathMap.insert(fontPath, absFontPath);
                continue;
            }

            fontPath = QStringLiteral("fonts/") + font + TTF_EXT;
            absFontPath = uipSrc.path() + QStringLiteral("/") + fontPath;
            if (QFile::exists(absFontPath)) {
                if (!outPathMap.contains(fontPath))
                    outPathMap.insert(fontPath, absFontPath);
                continue;
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
    QFile file(spPath);

    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return false;
    }

    QDomDocument domDoc;
    domDoc.setContent(&file);

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
