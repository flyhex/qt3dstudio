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

#include "ProjectFile.h"
#include "Qt3DSFileTools.h"
#include "Exceptions.h"
#include "DataInputDlg.h"
#include "StudioApp.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Core.h"
#include "Doc.h"
#include "PresentationFile.h"
#include "IStudioRenderer.h"
#include <QtCore/qdiriterator.h>
#include <QtXml/qdom.h>

ProjectFile::ProjectFile()
{

}

// find the 1st .uia file in the current or parent directories and assume this is the project file,
// as a project should have only 1 .uia file
void ProjectFile::ensureProjectFile()
{
    if (!m_fileInfo.exists()) {
        QFileInfo uipInfo(g_StudioApp.GetCore()->GetDoc()->GetDocumentPath());
        QString uiaPath(PresentationFile::findProjectFile(uipInfo.absoluteFilePath()));

        if (uiaPath.isEmpty()) {
            // .uia not found, create a new one in the same folder as uip. Creation sets file info.
            create(uipInfo.absoluteFilePath().replace(QLatin1String(".uip"),
                                                      QLatin1String(".uia")));
            addPresentationNode(uipInfo.absoluteFilePath());
            updateDocPresentationId();
        } else {
            // .uia found, set project file info
            m_fileInfo.setFile(uiaPath);
        }
    }
}

void ProjectFile::initProjectFile(const QString &presPath)
{
    QFileInfo uipFile(presPath);
    QString uiaPath(PresentationFile::findProjectFile(uipFile.absoluteFilePath()));

    if (uiaPath.isEmpty()) {
        // .uia not found, clear project file info
        m_fileInfo = QFileInfo();
    } else {
        // .uia found, set project file info
        m_fileInfo.setFile(uiaPath);
    }
}

/**
 * Add a presentation or presentation-qml node to the project file
 *
 * @param pPath the absolute path to the presentation file, it will be saved as relative
 * @param pId presentation Id
 */
void ProjectFile::addPresentationNode(const QString &pPath, const QString &pId)
{
    addPresentationNodes({{pPath, pId}});
}

// Add a list of presentation or presentation-qml nodes to the project file
void ProjectFile::addPresentationNodes(const QHash<QString, QString> &nodeList)
{
    ensureProjectFile();

    // open the uia file
    QFile file(getProjectFilePath());
    file.open(QIODevice::ReadWrite);
    QDomDocument doc;
    doc.setContent(&file);

    QDomElement rootElem = doc.documentElement();
    QDomElement assetsElem = rootElem.firstChildElement(QStringLiteral("assets"));

    // create the <assets> node if it doesn't exist
    bool initial = false;
    if (assetsElem.isNull()) {
        assetsElem = doc.createElement(QStringLiteral("assets"));
        rootElem.insertBefore(assetsElem, {});
        initial = true;
    }

    QHash<QString, QString> changesList;
    QHashIterator<QString, QString> nodesIt(nodeList);
    while (nodesIt.hasNext()) {
        nodesIt.next();
        const QString presPath = nodesIt.key();
        const QString presId = nodesIt.value();
        QString relativePresentationPath
                = QDir(getProjectPath()).relativeFilePath(presPath);

        // make sure the node doesn't already exist
        bool nodeExists = false;
        for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
            p = p.nextSibling().toElement()) {
            if ((p.nodeName() == QLatin1String("presentation")
                 || p.nodeName() == QLatin1String("presentation-qml"))
                    && p.attribute(QStringLiteral("src")) == relativePresentationPath) {
                nodeExists = true;
                break;
            }
        }

        if (!nodeExists) {
            const QString presentationId
                    = ensureUniquePresentationId(presId.isEmpty()
                                                 ? QFileInfo(presPath).completeBaseName()
                                                 : presId);

            if (assetsElem.attribute(QStringLiteral("initial")).isEmpty()) {
                assetsElem.setAttribute(QStringLiteral("initial"), presentationId);
                m_initialPresentation = presentationId;
            }

            // add the presentation node
            bool isQml = presPath.endsWith(QLatin1String(".qml"));
            QDomElement pElem = isQml ? doc.createElement(QStringLiteral("presentation-qml"))
                                      : doc.createElement(QStringLiteral("presentation"));
            pElem.setAttribute(QStringLiteral("id"), presentationId);
            pElem.setAttribute(isQml ? QStringLiteral("args") : QStringLiteral("src"),
                               relativePresentationPath);
            assetsElem.appendChild(pElem);
            changesList.insert(relativePresentationPath, presentationId);

            if (!initial) {
                g_StudioApp.m_subpresentations.push_back(
                            SubPresentationRecord(isQml ? QStringLiteral("presentation-qml")
                                                        : QStringLiteral("presentation"),
                                                  presentationId, relativePresentationPath));
            }
        }
    }

    if (initial || changesList.size() > 0) {
        file.resize(0);
        file.write(doc.toByteArray(4));
    }
    file.close();

    if (changesList.size() > 0) {
        g_StudioApp.getRenderer().RegisterSubpresentations(g_StudioApp.m_subpresentations);

        QHashIterator<QString, QString> changesIt(changesList);
        while (changesIt.hasNext()) {
            changesIt.next();
            Q_EMIT presentationIdChanged(changesIt.key(), changesIt.value());
        }
    }
}

// Get the src attribute (relative path) to the initial presentation in a uia file, if no initial
// presentation exists, the first one is returned.
QString ProjectFile::getInitialPresentationSrc(const QString &uiaPath)
{
    QFile file(uiaPath);
    file.open(QIODevice::ReadOnly);
    QDomDocument domDoc;
    domDoc.setContent(&file);
    file.close();

    QString firstPresentationSrc;
    QDomElement assetsElem = domDoc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        QString initialId = assetsElem.attribute(QStringLiteral("initial"));
        if (!initialId.isEmpty()) {
            QDomNodeList pNodes = assetsElem.elementsByTagName(QStringLiteral("presentation"));
            for (int i = 0; i < pNodes.count(); ++i) {
                QDomElement pElem = pNodes.at(i).toElement();
                if (pElem.attribute(QStringLiteral("id")) == initialId)
                    return pElem.attribute(QStringLiteral("src"));

                if (i == 0)
                    firstPresentationSrc = pElem.attribute(QStringLiteral("src"));
            }
        }
    }

    return firstPresentationSrc;
}

/**
 * Write a presentation id to the project file.
 * If the presentation id doesn't exist yet in project, it's added.
 *
 * This also updates the Doc presentation Id if the src param is empty
 * or same as current presentation.
 *
 * @param id presentation Id
 * @param src source node, if empty the current document node is used
 */
void ProjectFile::writePresentationId(const QString &id, const QString &src)
{
    ensureProjectFile();

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    QString theSrc = src.isEmpty() ? doc->getRelativePath() : src;
    QString theId = id.isEmpty() ? doc->getPresentationId() : id;
    bool isQml = theSrc.endsWith(QLatin1String(".qml"));

    if (theSrc == doc->getRelativePath())
        doc->setPresentationId(id);

    QFile file(getProjectFilePath());
    file.open(QIODevice::ReadWrite);
    QDomDocument domDoc;
    domDoc.setContent(&file);

    QDomElement assetsElem = domDoc.documentElement().firstChildElement(QStringLiteral("assets"));
    QDomNodeList pqNodes = isQml ? assetsElem.elementsByTagName(QStringLiteral("presentation-qml"))
                                 : assetsElem.elementsByTagName(QStringLiteral("presentation"));

    QString oldId;
    if (!pqNodes.isEmpty()) {
        for (int i = 0; i < pqNodes.count(); ++i) {
            QDomElement pqElem = pqNodes.at(i).toElement();
            QString srcOrArgs = isQml ? pqElem.attribute(QStringLiteral("args"))
                                      : pqElem.attribute(QStringLiteral("src"));
            if (srcOrArgs == theSrc) {
                oldId = pqElem.attribute(QStringLiteral("id"));
                pqElem.setAttribute(QStringLiteral("id"), theId);

                if (assetsElem.attribute(QStringLiteral("initial")) == oldId) {
                    assetsElem.setAttribute(QStringLiteral("initial"), theId);
                    m_initialPresentation = theId;
                }
                break;
            }
        }
    }

    if (!src.isEmpty() && oldId.isEmpty()) { // new presentation, add it
        // overwrite the uia file
        file.resize(0);
        file.write(domDoc.toByteArray(4));
        file.close();
        QDir projectDir(getProjectPath());
        addPresentationNode(QDir::cleanPath(projectDir.absoluteFilePath(theSrc)), theId);
    } else if (!oldId.isEmpty()) { // the presentation id changed
        // overwrite the uia file
        file.resize(0);
        file.write(domDoc.toByteArray(4));

        // update m_subpresentations
        auto *sp = std::find_if(g_StudioApp.m_subpresentations.begin(),
                                g_StudioApp.m_subpresentations.end(),
                               [&theSrc](const SubPresentationRecord &spr) -> bool {
                                   return spr.m_argsOrSrc == theSrc;
                               });
        if (sp != g_StudioApp.m_subpresentations.end())
            sp->m_id = theId;

        // update current doc instances (layers and images) that are using this presentation Id
        auto *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
        qt3dsdm::IPropertySystem *propSystem = doc->GetStudioSystem()->GetPropertySystem();
        std::function<void(qt3dsdm::Qt3DSDMInstanceHandle)>
        parseChildren = [&](qt3dsdm::Qt3DSDMInstanceHandle instance) {
            Q3DStudio::CGraphIterator iter;
            GetAssetChildren(doc, instance, iter);

            while (!iter.IsDone()) {
                qt3dsdm::Qt3DSDMInstanceHandle child = iter.GetCurrent();
                if (bridge->GetObjectType(child) & (OBJTYPE_LAYER | OBJTYPE_IMAGE)) {
                    if (bridge->GetSourcePath(child).toQString() == oldId) {
                        propSystem->SetInstancePropertyValue(child, bridge->GetSourcePathProperty(),
                                                             qt3dsdm::SValue(QVariant(theId)));
                    }
                    if (bridge->getSubpresentation(child).toQString() == oldId) {
                        propSystem->SetInstancePropertyValue(child,
                                                             bridge->getSubpresentationProperty(),
                                                             qt3dsdm::SValue(QVariant(theId)));
                    }
                }
                parseChildren(child);
                ++iter;
            }
        };
        parseChildren(doc->GetSceneInstance());

        // update changed presentation Id in all .uip files if in-use
        QDomNodeList pNodes = assetsElem.elementsByTagName(QStringLiteral("presentation"));
        for (int i = 0; i < pNodes.count(); ++i) {
            QDomElement pElem = pNodes.at(i).toElement();
            QString path = QDir(getProjectPath())
                                        .absoluteFilePath(pElem.attribute(QStringLiteral("src")));
            PresentationFile::updatePresentationId(path, oldId, theId);
        }
        Q_EMIT presentationIdChanged(theSrc, theId);
    }
}

// Set the doc PresentationId from the project file, this is called after a document is loaded.
// If there is no project file, it simply clears the id.
void ProjectFile::updateDocPresentationId()
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    doc->setPresentationId({});

    if (!m_fileInfo.exists())
        return;

    QFile file(getProjectFilePath());
    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return;
    }

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (!reader.atEnd()) {
        if (reader.readNextStartElement() && reader.name() == QLatin1String("presentation")) {
            const auto attrs = reader.attributes();
            if (attrs.value(QLatin1String("src")) == doc->getRelativePath()) {
                // current presentation node
                doc->setPresentationId(attrs.value(QLatin1String("id")).toString());
                return;
            }
        }
    }
}

// get a presentationId that match a given src attribute
QString ProjectFile::getPresentationId(const QString &src) const
{
    if (!m_fileInfo.exists())
        return {};

    if (src == g_StudioApp.GetCore()->GetDoc()->getRelativePath()) {
        return g_StudioApp.GetCore()->GetDoc()->getPresentationId();
    } else {
        auto *sp = std::find_if(g_StudioApp.m_subpresentations.begin(),
                                g_StudioApp.m_subpresentations.end(),
                               [&src](const SubPresentationRecord &spr) -> bool {
                                   return spr.m_argsOrSrc == src;
                               });
        if (sp != g_StudioApp.m_subpresentations.end())
            return sp->m_id;
    }

    return {};
}

// create the project .uia file
void ProjectFile::create(const QString &uiaPath)
{
    QDomDocument doc;
    doc.setContent(QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                                  "<application xmlns=\"http://qt.io/qt3dstudio/uia\">"
                                    "<statemachine ref=\"#logic\">"
                                      "<visual-states>"
                                        "<state ref=\"Initial\">"
                                          "<enter>"
                                            "<goto-slide element=\"main:Scene\" rel=\"next\"/>"
                                          "</enter>"
                                        "</state>"
                                      "</visual-states>"
                                    "</statemachine>"
                                  "</application>"));

    QFile file(uiaPath);
    file.open(QIODevice::WriteOnly);
    file.resize(0);
    file.write(doc.toByteArray(4));
    file.close();

    m_fileInfo.setFile(uiaPath);
}

/**
 * Clone the project file with a preview suffix and set the initial attribute to the currently
 * open document
 *
 * @return path to the preview project file. Return path to .uip or preview .uip file if there
 * is no project file.
 */
QString ProjectFile::createPreview()
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    QString uipPrvPath = doc->GetDocumentPath();
    // create a preview uip if doc modified
    if (doc->IsModified()) {
        uipPrvPath.replace(QLatin1String(".uip"), QLatin1String("_@preview@.uip"));
        g_StudioApp.GetCore()->OnSaveDocument(uipPrvPath, true);
    }

    // if no project file exist (.uia) just return the preview uip path
    if (!m_fileInfo.exists())
        return uipPrvPath;

    // create a preview project file
    QString prvPath = getProjectFilePath();
    prvPath.replace(QLatin1String(".uia"), QLatin1String("_@preview@.uia"));

    if (QFile::exists(prvPath))
        QFile::remove(prvPath);

    if (QFile::copy(getProjectFilePath(), prvPath)) {
        QFile file(prvPath);
        file.open(QIODevice::ReadWrite);
        QDomDocument domDoc;
        domDoc.setContent(&file);

        QDomElement assetsElem = domDoc.documentElement()
                                 .firstChildElement(QStringLiteral("assets"));
        assetsElem.setAttribute(QStringLiteral("initial"), doc->getPresentationId());

        if (doc->IsModified()) {
            // Set the preview uip path in the uia file
            QDomNodeList pNodes = assetsElem.elementsByTagName(QStringLiteral("presentation"));
            for (int i = 0; i < pNodes.count(); ++i) {
                QDomElement pElem = pNodes.at(i).toElement();
                if (pElem.attribute(QStringLiteral("id")) == doc->getPresentationId()) {
                    QString src = QDir(getProjectPath()).relativeFilePath(uipPrvPath);
                    pElem.setAttribute(QStringLiteral("src"), src);
                    break;
                }
            }
        }

        file.resize(0);
        file.write(domDoc.toByteArray(4));

        return prvPath;
    } else {
        qWarning() << "Couldn't clone project file";
    }

    return {};
}

void ProjectFile::loadSubpresentationsAndDatainputs(
        QVector<SubPresentationRecord> &subpresentations,
        QMap<QString, CDataInputDialogItem *> &datainputs)
{
    if (!m_fileInfo.exists())
        return;

    subpresentations.clear();
    datainputs.clear();

    QFile file(getProjectFilePath());
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    m_initialPresentation = g_StudioApp.GetCore()->GetDoc()->getPresentationId();

    QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        QString initial = assetsElem.attribute(QStringLiteral("initial"));
        if (!initial.isEmpty())
            m_initialPresentation = initial;
        for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
            p = p.nextSibling().toElement()) {
            if ((p.nodeName() == QLatin1String("presentation")
                 || p.nodeName() == QLatin1String("presentation-qml"))
                    && p.attribute(QStringLiteral("id"))
                       != g_StudioApp.GetCore()->GetDoc()->getPresentationId()) {
                QString argsOrSrc = p.attribute(QStringLiteral("src"));
                if (argsOrSrc.isNull())
                    argsOrSrc = p.attribute(QStringLiteral("args"));
                subpresentations.push_back(
                            SubPresentationRecord(p.nodeName(), p.attribute("id"), argsOrSrc));
            } else if (p.nodeName() == QLatin1String("dataInput")) {
                CDataInputDialogItem *item = new CDataInputDialogItem();
                item->name = p.attribute(QStringLiteral("name"));
                QString type = p.attribute(QStringLiteral("type"));
                if (type == QLatin1String("Ranged Number")) {
                    item->type = EDataType::DataTypeRangedNumber;
                    item->minValue = p.attribute(QStringLiteral("min")).toFloat();
                    item->maxValue = p.attribute(QStringLiteral("max")).toFloat();
                } else if (type == QLatin1String("String")) {
                    item->type = EDataType::DataTypeString;
                } else if (type == QLatin1String("Float")) {
                    item->type = EDataType::DataTypeFloat;
                } else if (type == QLatin1String("Boolean")) {
                    item->type = EDataType::DataTypeBoolean;
                } else if (type == QLatin1String("Vector3")) {
                    item->type = EDataType::DataTypeVector3;
                } else if (type == QLatin1String("Vector2")) {
                    item->type = EDataType::DataTypeVector2;
                } else if (type == QLatin1String("Variant")) {
                    item->type = EDataType::DataTypeVariant;
                }
#ifdef DATAINPUT_EVALUATOR_ENABLED
                else if (type == QLatin1String("Evaluator")) {
                    item->type = EDataType::DataTypeEvaluator;
                    item->valueString = p.attribute(QStringLiteral("evaluator"));
                }
#endif
                datainputs.insert(item->name, item);
            }
        }
    }
}

/**
 * Check that a given presentation's or Qml stream's id is unique
 *
 * @param id presentation's or Qml stream's Id
 * @param src source node to exclude from the check, if empty the current document node is used
 */
bool ProjectFile::isUniquePresentationId(const QString &id, const QString &src) const
{
    if (!m_fileInfo.exists())
        return true;

    QString theSrc = src.isEmpty() ? g_StudioApp.GetCore()->GetDoc()->getRelativePath() : src;
    bool isCurrDoc = theSrc == g_StudioApp.GetCore()->GetDoc()->getRelativePath();

    if (!isCurrDoc && id == g_StudioApp.GetCore()->GetDoc()->getPresentationId())
        return false;

    auto *sp = std::find_if(g_StudioApp.m_subpresentations.begin(),
                            g_StudioApp.m_subpresentations.end(),
                           [&id, &theSrc](const SubPresentationRecord &spr) -> bool {
                               return spr.m_id == id && spr.m_argsOrSrc != theSrc;
                           });
    return  sp == g_StudioApp.m_subpresentations.end();
}

QString ProjectFile::ensureUniquePresentationId(const QString &id) const
{
    if (!m_fileInfo.exists())
        return id;

    QFile file(m_fileInfo.filePath());
    file.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&file);
    file.close();
    QString newId = id;
    QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
    if (!assetsElem.isNull()) {
        bool unique;
        int n = 1;
        do {
            unique = true;
            for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
                p = p.nextSibling().toElement()) {
                if ((p.nodeName() == QLatin1String("presentation")
                     || p.nodeName() == QLatin1String("presentation-qml"))
                        && p.attribute(QStringLiteral("id")) == newId) {
                    newId = id + QString::number(n++);
                    unique = false;
                    break;
                }
            }
        } while (!unique);
    }

    return newId;
}

// Get the path to the project root. If .uia doesn't exist, return path to current presentation.
QString ProjectFile::getProjectPath() const
{
    if (m_fileInfo.exists())
        return m_fileInfo.path();
    else
        return QFileInfo(g_StudioApp.GetCore()->GetDoc()->GetDocumentPath()).absolutePath();
}

// Get the path to the project's .uia file. If .uia doesn't exist, return empty string.
QString ProjectFile::getProjectFilePath() const
{
    if (m_fileInfo.exists())
        return m_fileInfo.filePath();
    else
        return {};
}

// Returns current project name or empty string if there is no .uia file
QString ProjectFile::getProjectName() const
{
    if (m_fileInfo.exists())
        return m_fileInfo.completeBaseName();
    else
        return {};
}

/**
 * Get presentations out of a uia file
 *
 * @param inUiaPath uia file path
 * @param outSubpresentations list of collected presentations
 * @param excludePresentationSrc execluded presentation, (commonly the current presentation)
 */
// static
void ProjectFile::getPresentations(const QString &inUiaPath,
                                   QVector<SubPresentationRecord> &outSubpresentations,
                                   const QString &excludePresentationSrc)
{
    QFile file(inUiaPath);
    file.open(QFile::Text | QFile::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << file.errorString();
        return;
    }

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (!reader.atEnd()) {
        if (reader.readNextStartElement()
            && (reader.name() == QLatin1String("presentation")
                || reader.name() == QLatin1String("presentation-qml"))) {
            const auto attrs = reader.attributes();
            QString argsOrSrc = attrs.value(QLatin1String("src")).toString();
            if (excludePresentationSrc == argsOrSrc)
                continue;
            if (argsOrSrc.isNull())
                argsOrSrc = attrs.value(QLatin1String("args")).toString();

            outSubpresentations.push_back(
                        SubPresentationRecord(reader.name().toString(),
                                              attrs.value(QLatin1String("id")).toString(),
                                              argsOrSrc));
        } else if (reader.name() == QLatin1String("assets") && !reader.isStartElement()) {
            break; // reached end of <assets>
        }
    }
}

void ProjectFile::setInitialPresentation(const QString &initialId)
{
    if (!initialId.isEmpty() && m_initialPresentation != initialId) {
        m_initialPresentation = initialId;

        ensureProjectFile();

        QFile file(getProjectFilePath());
        file.open(QIODevice::ReadWrite);
        QDomDocument domDoc;
        domDoc.setContent(&file);

        QDomElement assetsElem
                = domDoc.documentElement().firstChildElement(QStringLiteral("assets"));
        if (!assetsElem.isNull() && assetsElem.attribute(QStringLiteral("initial"))
                != m_initialPresentation) {
            assetsElem.setAttribute(QStringLiteral("initial"), m_initialPresentation);

            // Rewrite project file
            file.resize(0);
            file.write(domDoc.toByteArray(4));
        }
    }
}

// Returns true if file rename was successful. The parameters are relative to project root.
bool ProjectFile::renamePresentationFile(const QString &oldName, const QString &newName)
{
    const QString fullOldPath = getAbsoluteFilePathTo(oldName);
    const QString fullNewPath = getAbsoluteFilePathTo(newName);
    QFile presFile(fullOldPath);
    const bool success = presFile.rename(fullNewPath);

    if (success) {
        // Update assets in .uia
        ensureProjectFile();

        const bool isQml = oldName.endsWith(QLatin1String(".qml"));

        if (isQml && g_StudioApp.m_qmlStreamMap.contains(fullOldPath)) {
            // Update Qml stream type cache
            g_StudioApp.m_qmlStreamMap.remove(fullOldPath);
            g_StudioApp.m_qmlStreamMap.insert(fullNewPath, true);
        }

        QFile file(getProjectFilePath());
        file.open(QIODevice::ReadWrite);
        QDomDocument domDoc;
        domDoc.setContent(&file);

        QDomElement assetsElem
                = domDoc.documentElement().firstChildElement(QStringLiteral("assets"));
        if (!assetsElem.isNull()) {
            QDomNodeList pqNodes
                    = isQml ? assetsElem.elementsByTagName(QStringLiteral("presentation-qml"))
                            : assetsElem.elementsByTagName(QStringLiteral("presentation"));
            if (!pqNodes.isEmpty()) {
                CDoc *doc = g_StudioApp.GetCore()->GetDoc();
                for (int i = 0; i < pqNodes.count(); ++i) {
                    QDomElement pqElem = pqNodes.at(i).toElement();
                    const QString attTag = isQml ? QStringLiteral("args") : QStringLiteral("src");
                    const QString srcOrArgs = pqElem.attribute(attTag);
                    if (srcOrArgs == oldName) {
                        pqElem.setAttribute(attTag, newName);

                        if (pqElem.attribute(QStringLiteral("id")) != doc->getPresentationId()) {
                            // update m_subpresentations
                            auto *sp = std::find_if(
                                        g_StudioApp.m_subpresentations.begin(),
                                        g_StudioApp.m_subpresentations.end(),
                                        [&oldName](const SubPresentationRecord &spr) -> bool {
                                            return spr.m_argsOrSrc == oldName;
                                        });
                            if (sp != g_StudioApp.m_subpresentations.end())
                                sp->m_argsOrSrc = newName;
                        } else {
                            // If renaming current presentation, need to update the doc path, too
                            doc->SetDocumentPath(fullNewPath);
                        }

                        file.resize(0);
                        file.write(domDoc.toByteArray(4));

                        Q_EMIT assetNameChanged();
                        break;
                    }
                }
            }
        }
    }

    return success;
}

/**
 * Returns an absolute file path for a given relative file path.
 *
 * @param relFilePath A file path relative to project root to convert.
 */
QString ProjectFile::getAbsoluteFilePathTo(const QString &relFilePath) const
{
    auto projectPath = QDir(getProjectPath()).absoluteFilePath(relFilePath);
    return QDir::cleanPath(projectPath);
}

/**
 * Returns a file path relative to the project root for given absolute file path.
 *
 * @param absFilePath An absolute file path to convert.
 */
QString ProjectFile::getRelativeFilePathTo(const QString &absFilePath) const
{
    return QDir(getProjectPath()).relativeFilePath(absFilePath);
}

// Return multimap of type subpresentationid - QPair<datainput, propertyname>
QMultiMap<QString, QPair<QString, QString>>
ProjectFile::getDiBindingtypesFromSubpresentations() const
{
    QMultiMap<QString, QPair<QString, QString>> map;
    for (auto sp : qAsConst(g_StudioApp.m_subpresentations))
        PresentationFile::getDataInputBindings(sp, map);

    return map;
}
