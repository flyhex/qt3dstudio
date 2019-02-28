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
#include "IDocumentEditor.h"
#include "PresentationFile.h"
#include "IStudioRenderer.h"
#include "StudioUtils.h"
#include "Dispatch.h"
#include <QtCore/qdiriterator.h>
#include <QtCore/qsavefile.h>
#include <QtCore/qtimer.h>
#include <QtCore/qrandom.h>
#include <QtWidgets/qmessagebox.h>

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

    QDomDocument doc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, doc))
        return;

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

    if (initial || changesList.size() > 0)
        StudioUtils::commitDomDocumentSave(file, doc);

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
// presentation exists, the first one is returned. Returns empty string if file cannot be read.
QString ProjectFile::getInitialPresentationSrc(const QString &uiaPath)
{
    QDomDocument domDoc;
    if (!StudioUtils::readFileToDomDocument(uiaPath, domDoc))
        return {};

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

    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

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
        StudioUtils::commitDomDocumentSave(file, domDoc);
        QDir projectDir(getProjectPath());
        addPresentationNode(QDir::cleanPath(projectDir.absoluteFilePath(theSrc)), theId);
    } else if (!oldId.isEmpty()) { // the presentation id changed
        StudioUtils::commitDomDocumentSave(file, domDoc);

        // update m_subpresentations
        auto *sp = std::find_if(g_StudioApp.m_subpresentations.begin(),
                                g_StudioApp.m_subpresentations.end(),
                               [&theSrc](const SubPresentationRecord &spr) -> bool {
                                   return spr.m_argsOrSrc == theSrc;
                               });
        if (sp != g_StudioApp.m_subpresentations.end())
            sp->m_id = theId;

        // update current doc instances (layers and images) that are using this presentation Id
        qt3dsdm::TInstanceHandleList instancesToRefresh;
        auto *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
        qt3dsdm::IPropertySystem *propSystem = doc->GetStudioSystem()->GetPropertySystem();
        std::function<void(qt3dsdm::Qt3DSDMInstanceHandle)>
        parseChildren = [&](qt3dsdm::Qt3DSDMInstanceHandle instance) {
            Q3DStudio::CGraphIterator iter;
            GetAssetChildren(doc, instance, iter);

            while (!iter.IsDone()) {
                qt3dsdm::Qt3DSDMInstanceHandle child = iter.GetCurrent();
                if (bridge->GetObjectType(child) & (OBJTYPE_LAYER | OBJTYPE_IMAGE)) {
                    bool add = false;
                    if (bridge->GetSourcePath(child).toQString() == oldId) {
                        propSystem->SetInstancePropertyValue(child, bridge->GetSourcePathProperty(),
                                                             qt3dsdm::SValue(QVariant(theId)));
                        add = true;
                    }
                    if (bridge->getSubpresentation(child).toQString() == oldId) {
                        propSystem->SetInstancePropertyValue(child,
                                                             bridge->getSubpresentationProperty(),
                                                             qt3dsdm::SValue(QVariant(theId)));
                        add = true;
                    }
                    if (add)
                        instancesToRefresh.push_back(child);
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

        g_StudioApp.getRenderer().RegisterSubpresentations(g_StudioApp.m_subpresentations);
        if (instancesToRefresh.size() > 0) {
            g_StudioApp.GetCore()->GetDispatch()->FireImmediateRefreshInstance(
                        &(instancesToRefresh[0]), long(instancesToRefresh.size()));
            for (auto &instance : instancesToRefresh)
                doc->getSceneEditor()->saveIfMaterial(instance);
        }
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
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
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
    QDomDocument domDoc;
    domDoc.setContent(QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
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

    QSaveFile file(uiaPath);
    if (StudioUtils::openTextSave(file)) {
        StudioUtils::commitDomDocumentSave(file, domDoc);
        m_fileInfo.setFile(uiaPath);
    }
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
        QDomDocument domDoc;
        QSaveFile file(prvPath);
        if (StudioUtils::openDomDocumentSave(file, domDoc)) {
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
            StudioUtils::commitDomDocumentSave(file, domDoc);
        }

        return prvPath;
    } else {
        qWarning() << "Couldn't clone project file";
    }

    return {};
}

void ProjectFile::parseDataInputElem(const QDomElement &elem,
                                     QMap<QString, CDataInputDialogItem *> &dataInputs)
{
    if (elem.nodeName() == QLatin1String("dataInput")) {
        CDataInputDialogItem *item = new CDataInputDialogItem();
        item->name = elem.attribute(QStringLiteral("name"));
        QString type = elem.attribute(QStringLiteral("type"));
        if (type == QLatin1String("Ranged Number")) {
            item->type = EDataType::DataTypeRangedNumber;
            item->minValue = elem.attribute(QStringLiteral("min")).toFloat();
            item->maxValue = elem.attribute(QStringLiteral("max")).toFloat();
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
            item->valueString = elem.attribute(QStringLiteral("evaluator"));
        }
#endif
        item->metaDataKey = elem.attribute((QStringLiteral("metadatakey")));
        item->metaData = elem.attribute((QStringLiteral("metadata")));
        dataInputs.insert(item->name, item);
    }
}

void ProjectFile::loadDataInputs(const QString &projFile,
                                 QMap<QString, CDataInputDialogItem *> &dataInputs)
{
    QFileInfo fi(projFile);
    if (fi.exists()) {
        QDomDocument doc;
        if (!StudioUtils::readFileToDomDocument(projFile, doc))
            return;
        QDomElement assetsElem = doc.documentElement().firstChildElement(QStringLiteral("assets"));
        if (!assetsElem.isNull()) {
            for (QDomElement p = assetsElem.firstChild().toElement(); !p.isNull();
                p = p.nextSibling().toElement()) {
                parseDataInputElem(p, dataInputs);
            }
        }
    }
}

void ProjectFile::loadSubpresentationsAndDatainputs(
        QVector<SubPresentationRecord> &subpresentations,
        QMap<QString, CDataInputDialogItem *> &datainputs)
{
    if (!m_fileInfo.exists())
        return;

    subpresentations.clear();
    datainputs.clear();

    m_initialPresentation = g_StudioApp.GetCore()->GetDoc()->getPresentationId();

    QDomDocument doc;
    if (!StudioUtils::readFileToDomDocument(getProjectFilePath(), doc))
        return;

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
            } else {
                parseDataInputElem(p, datainputs);
            }
        }
    }
    g_StudioApp.GetCore()->GetDoc()->UpdateDatainputMap();
}

/**
 * Check that a given presentation's or Qml stream's id is unique
 *
 * @param id presentation's or Qml stream's Id
 * @param src source node to exclude from the check. Defaults to empty.
 */
bool ProjectFile::isUniquePresentationId(const QString &id, const QString &src) const
{
    if (!m_fileInfo.exists())
        return true;

    bool isCurrDoc = src == g_StudioApp.GetCore()->GetDoc()->getRelativePath();

    if (!isCurrDoc && id == g_StudioApp.GetCore()->GetDoc()->getPresentationId())
        return false;

    auto *sp = std::find_if(g_StudioApp.m_subpresentations.begin(),
                            g_StudioApp.m_subpresentations.end(),
                           [&id, &src](const SubPresentationRecord &spr) -> bool {
                               return spr.m_id == id && spr.m_argsOrSrc != src;
                           });
    return  sp == g_StudioApp.m_subpresentations.end();
}

// Returns unique presentation name based on given relative presentation path
// Only the file name base is returned, no path or suffix.
QString ProjectFile::getUniquePresentationName(const QString &presSrc) const
{
    if (!m_fileInfo.exists())
        return {};

    const QString fullPresSrc = getAbsoluteFilePathTo(presSrc);
    QFileInfo fi(fullPresSrc);
    const QStringList files = fi.dir().entryList(QDir::Files);
    QString checkName = fi.fileName();
    if (files.contains(checkName)) {
        const QString nameTemplate = QStringLiteral("%1%2.%3");
        const QString suffix = fi.suffix();
        QString base = fi.completeBaseName();
        int counter = 0;
        int checkIndex = base.size();
        while (checkIndex > 1 && base.at(checkIndex - 1).isDigit())
            --checkIndex;
        if (checkIndex < base.size())
            counter = base.mid(checkIndex).toInt();

        if (counter > 0)
            base = base.left(checkIndex);

        while (files.contains(checkName))
            checkName = nameTemplate.arg(base).arg(++counter).arg(suffix);
    }

    return QFileInfo(checkName).completeBaseName();
}

QString ProjectFile::ensureUniquePresentationId(const QString &id) const
{
    if (!m_fileInfo.exists())
        return id;

    QDomDocument doc;
    if (!StudioUtils::readFileToDomDocument(m_fileInfo.filePath(), doc))
        return id;

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
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
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

        QDomDocument domDoc;
        QSaveFile file(getProjectFilePath());
        if (!StudioUtils::openDomDocumentSave(file, domDoc))
            return;

        QDomElement assetsElem
                = domDoc.documentElement().firstChildElement(QStringLiteral("assets"));
        if (!assetsElem.isNull() && assetsElem.attribute(QStringLiteral("initial"))
                != m_initialPresentation) {
            assetsElem.setAttribute(QStringLiteral("initial"), m_initialPresentation);

            StudioUtils::commitDomDocumentSave(file, domDoc);
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

        QDomDocument domDoc;
        QSaveFile file(getProjectFilePath());
        if (!StudioUtils::openDomDocumentSave(file, domDoc))
            return false;

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

                        StudioUtils::commitDomDocumentSave(file, domDoc);

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
 * Delete a presentation (or qml-stream) file and remove references to it from the project file.
 * This function assumes the removed presentation is not referenced by any presentation
 * in the project and is not the current presentation.
 *
 * @param filePath Absolute file path to presentation to delete
 */
void ProjectFile::deletePresentationFile(const QString &filePath)
{
    QFile(filePath).remove();

    if (m_fileInfo.exists()) {
        const QString relPath = getRelativeFilePathTo(filePath);
        const bool isQml = relPath.endsWith(QLatin1String(".qml"));

        // Update records and caches
        if (isQml && g_StudioApp.m_qmlStreamMap.contains(filePath))
            g_StudioApp.m_qmlStreamMap.remove(filePath);
        for (int i = 0, count = g_StudioApp.m_subpresentations.size(); i < count; ++i) {
            SubPresentationRecord &rec = g_StudioApp.m_subpresentations[i];
            if (rec.m_argsOrSrc == relPath) {
                g_StudioApp.m_subpresentations.remove(i);
                break;
            }
        }

        // Update project file
        QDomDocument domDoc;
        QSaveFile projectFile(getProjectFilePath());
        if (!StudioUtils::openDomDocumentSave(projectFile, domDoc))
            return;

        QDomElement assetsElem
                = domDoc.documentElement().firstChildElement(QStringLiteral("assets"));
        if (!assetsElem.isNull()) {
            QDomNodeList pqNodes
                    = isQml ? assetsElem.elementsByTagName(QStringLiteral("presentation-qml"))
                            : assetsElem.elementsByTagName(QStringLiteral("presentation"));
            if (!pqNodes.isEmpty()) {
                for (int i = 0; i < pqNodes.count(); ++i) {
                    QDomElement pqElem = pqNodes.at(i).toElement();
                    const QString attTag = isQml ? QStringLiteral("args") : QStringLiteral("src");
                    const QString srcOrArgs = pqElem.attribute(attTag);
                    if (srcOrArgs == relPath) {
                        const QString id = pqElem.attribute(QStringLiteral("id"));
                        // If initial presentation is deleted, change current to initial
                        if (assetsElem.attribute(QStringLiteral("initial")) == id) {
                            m_initialPresentation
                                    = g_StudioApp.GetCore()->GetDoc()->getPresentationId();
                            assetsElem.setAttribute(QStringLiteral("initial"),
                                                    m_initialPresentation);
                        }
                        assetsElem.removeChild(pqNodes.at(i));
                        StudioUtils::commitDomDocumentSave(projectFile, domDoc);
                        break;
                    }
                }
            }
        }
        // Update registrations asynchronously, as it messes with event processing, which can
        // cause issues with file models elsewhere in the editor unless file removal is fully
        // handled.
        QTimer::singleShot(0, []() {
            g_StudioApp.getRenderer().RegisterSubpresentations(g_StudioApp.m_subpresentations);
        });
    }
}

void ProjectFile::renameMaterial(const QString &oldName, const QString &newName)
{
    for (auto &pres : qAsConst(g_StudioApp.m_subpresentations)) {
        if (pres.m_type == QLatin1String("presentation")) {
            PresentationFile::renameMaterial(getAbsoluteFilePathTo(pres.m_argsOrSrc),
                                             oldName, newName);
        }
    }
    Q_EMIT assetNameChanged();
}

// Copies oldPres presentation as newPres. The id for newPres will be autogenerated.
bool ProjectFile::duplicatePresentation(const QString &oldPres, const QString &newPres)
{
    const QString fullOldPath = getAbsoluteFilePathTo(oldPres);
    const QString fullNewPath = getAbsoluteFilePathTo(newPres);
    const bool success = QFile::copy(fullOldPath, fullNewPath);

    if (success)
        addPresentationNode(fullNewPath, {});

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

/**
 * Load variants data to m_variantsDef
 *
 * @param filePath the file path to load the variants from. If empty, variants are loaded from the
 *                 project file and replace m_variantsDef. If a filePath is specified, the loaded
 *                 variants are merged with m_variantsDef
 */
void ProjectFile::loadVariants(const QString &filePath)
{
    if (!m_fileInfo.exists())
        return;

    bool isProj = filePath.isEmpty() || filePath == getProjectFilePath();
    QFile file(isProj ? getProjectFilePath() : filePath);
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        qWarning() << file.errorString();
        return;
    }

    if (isProj)
        m_variantsDef.clear();

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    VariantGroup *currentGroup = nullptr;
    while (!reader.atEnd()) {
        if (reader.readNextStartElement()) {
            if (reader.name() == QLatin1String("variantgroup")) {
                QString groupId = reader.attributes().value(QLatin1String("id")).toString();
                QString groupColor = reader.attributes().value(QLatin1String("color")).toString();
                currentGroup = &m_variantsDef[groupId];
                currentGroup->m_color = groupColor;
            } else if (reader.name() == QLatin1String("variant")) {
                if (currentGroup) {
                    QString tagId = reader.attributes().value(QLatin1String("id")).toString();
                    if (!currentGroup->m_tags.contains(tagId))
                        currentGroup->m_tags.append(tagId);
                } else {
                    qWarning() << "Error parsing variant tags.";
                }
            } else if (currentGroup) {
                break;
            }
        }
    }

    if (!isProj) {
        // if loading variants from a file, update the uia
        QDomDocument domDoc;
        QSaveFile fileProj(getProjectFilePath());
        if (!StudioUtils::openDomDocumentSave(fileProj, domDoc))
            return;

        QDomElement vElem = domDoc.documentElement().firstChildElement(QStringLiteral("variants"));
        if (!vElem.isNull())
            domDoc.documentElement().removeChild(vElem);

        vElem = domDoc.createElement(QStringLiteral("variants"));
        domDoc.documentElement().appendChild(vElem);

        const auto keys = m_variantsDef.keys();
        for (auto &g : keys) {
            QDomElement gElem = domDoc.createElement(QStringLiteral("variantgroup"));
            gElem.setAttribute(QStringLiteral("id"), g);
            gElem.setAttribute(QStringLiteral("color"), m_variantsDef[g].m_color);
            vElem.appendChild(gElem);

            for (auto &t : qAsConst(m_variantsDef[g].m_tags)) {
                QDomElement tElem = domDoc.createElement(QStringLiteral("variant"));
                tElem.setAttribute(QStringLiteral("id"), t);
                gElem.appendChild(tElem);
            }
        }

        StudioUtils::commitDomDocumentSave(fileProj, domDoc);
    }
}

// Add a new tag to a variants group
void ProjectFile::addVariantTag(const QString &group, const QString &newTag)
{
    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomElement newTagElem = domDoc.createElement(QStringLiteral("variant"));
    newTagElem.setAttribute(QStringLiteral("id"), newTag);

    QDomNodeList groupsElems = domDoc.documentElement()
                                     .firstChildElement(QStringLiteral("variants"))
                                     .elementsByTagName(QStringLiteral("variantgroup"));

    // update and save the uia
    for (int i = 0; i < groupsElems.count(); ++i) {
        QDomElement gElem = groupsElems.at(i).toElement();
        if (gElem.attribute(QStringLiteral("id")) == group) {
            gElem.appendChild(newTagElem);
            StudioUtils::commitDomDocumentSave(file, domDoc);
            break;
        }
    }

    // update m_variantsDef
    m_variantsDef[group].m_tags.append(newTag);
}

// Add a new group, it is assumes that the new group name is unique
void ProjectFile::addVariantGroup(const QString &newGroup)
{
    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomElement variantsElem = domDoc.documentElement()
                                     .firstChildElement(QStringLiteral("variants"));

    if (variantsElem.isNull()) {
        QDomElement newVariantsElem = domDoc.createElement(QStringLiteral("variants"));
        domDoc.documentElement().appendChild(newVariantsElem);
        variantsElem = newVariantsElem;
    }

    // generate random semi-bright color
    int r = 0x555555 + QRandomGenerator::global()->generate() % 0x555555; // 0x555555 = 0xffffff / 3
    QString newColor = QLatin1Char('#') + QString::number(r, 16);

    QDomElement newGroupElem = domDoc.createElement(QStringLiteral("variantgroup"));
    newGroupElem.setAttribute(QStringLiteral("id"), newGroup);
    newGroupElem.setAttribute(QStringLiteral("color"), newColor);
    variantsElem.appendChild(newGroupElem);
    StudioUtils::commitDomDocumentSave(file, domDoc);

    // update m_variantsDef
    VariantGroup g;
    g.m_color = newColor;
    m_variantsDef[newGroup] = g;
}

void ProjectFile::renameVariantTag(const QString &group, const QString &oldTag,
                                   const QString &newTag)
{
    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    // rename the tag in all uip files
    QDomNodeList presElems = domDoc.documentElement()
                                   .firstChildElement(QStringLiteral("assets"))
                                   .elementsByTagName(QStringLiteral("presentation"));
    for (int i = 0; i < presElems.count(); ++i) {
        QString pPath = m_fileInfo.path() + QLatin1Char('/')
                + presElems.at(i).toElement().attribute(QStringLiteral("src"));
        renameTagInUip(pPath, group, oldTag, newTag);
    }

    // update and save the uia
    QDomNodeList groupsElems = domDoc.documentElement()
                                     .firstChildElement(QStringLiteral("variants"))
                                     .elementsByTagName(QStringLiteral("variantgroup"));

    bool renamed = false;
     for (int i = 0; i < groupsElems.count(); ++i) {
        QDomElement gElem = groupsElems.at(i).toElement();
        if (gElem.attribute(QStringLiteral("id")) == group) {
            QDomNodeList tagsElems = gElem.childNodes();
            for (int j = 0; j < tagsElems.count(); ++j) {
                QDomElement tElem = tagsElems.at(j).toElement();
                if (tElem.attribute(QStringLiteral("id")) == oldTag) {
                    tElem.setAttribute(QStringLiteral("id"), newTag);
                    StudioUtils::commitDomDocumentSave(file, domDoc);
                    renamed = true;
                    break;
                }
            }
            if (renamed)
                break;
        }
    }

    // update the property
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    const auto layers = doc->getLayers();
    auto property = bridge->GetLayer().m_variants;
    for (auto layer : layers) {
        qt3dsdm::SValue sValue;
        if (propertySystem->GetInstancePropertyValue(layer, property, sValue)) {
            QString propVal = qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->toQString();
            QString oldGroupTagPair = QStringLiteral("%1:%2").arg(group).arg(oldTag);
            if (propVal.contains(oldGroupTagPair)) {
                propVal.replace(oldGroupTagPair, QStringLiteral("%1:%2").arg(group).arg(newTag));
                qt3dsdm::SValue sVal
                    = std::make_shared<qt3dsdm::CDataStr>(Q3DStudio::CString::fromQString(propVal));
                propertySystem->SetInstancePropertyValue(layer, property, sVal);
            }
        }
    }

    // update m_variantsDef
    for (auto &t : m_variantsDef[group].m_tags) {
        if (t == oldTag) {
            t = newTag;
            renamed = true;
            break;
        }
    }
}

// rename a variant group, newGroup is assumed to be unique
void ProjectFile::renameVariantGroup(const QString &oldGroup, const QString &newGroup)
{
    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    // rename the group in all uip files
    QDomNodeList presElems = domDoc.documentElement()
                                   .firstChildElement(QStringLiteral("assets"))
                                   .elementsByTagName(QStringLiteral("presentation"));
    for (int i = 0; i < presElems.count(); ++i) {
        QString pPath = m_fileInfo.path() + QLatin1Char('/')
                + presElems.at(i).toElement().attribute(QStringLiteral("src"));
        renameGroupInUip(pPath, oldGroup, newGroup);
    }

    // update and save the uia
    QDomNodeList groupsElems = domDoc.documentElement()
                                     .firstChildElement(QStringLiteral("variants"))
                                     .elementsByTagName(QStringLiteral("variantgroup"));

     for (int i = 0; i < groupsElems.count(); ++i) {
        QDomElement gElem = groupsElems.at(i).toElement();
        if (gElem.attribute(QStringLiteral("id")) == oldGroup) {
            gElem.setAttribute(QStringLiteral("id"), newGroup);
            StudioUtils::commitDomDocumentSave(file, domDoc);
            break;
        }
    }

     // update the property
     CDoc *doc = g_StudioApp.GetCore()->GetDoc();
     const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
     const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
     const auto layers = doc->getLayers();
     auto property = bridge->GetLayer().m_variants;
     for (auto layer : layers) {
         qt3dsdm::SValue sValue;
         if (propertySystem->GetInstancePropertyValue(layer, property, sValue)) {
             QString propVal = qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->toQString();
             QString oldGroupWithColon = QStringLiteral("%1:").arg(oldGroup);
             if (propVal.contains(oldGroupWithColon)) {
                 propVal.replace(oldGroupWithColon, QStringLiteral("%1:").arg(newGroup));
                 qt3dsdm::SValue sVal = std::make_shared<qt3dsdm::CDataStr>(
                                        Q3DStudio::CString::fromQString(propVal));
                 propertySystem->SetInstancePropertyValue(layer, property, sVal);
             }
         }
     }

    // update m_variantsDef
    m_variantsDef[newGroup] = m_variantsDef[oldGroup];
    m_variantsDef.remove(oldGroup);
}

void ProjectFile::deleteVariantGroup(const QString &group)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();

    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    // check if group is in use in other presentations in the porject
    int inUseIdx = -1; // index of first presentation that has the group in-use
    QDomNodeList presElems = domDoc.documentElement()
                                   .firstChildElement(QStringLiteral("assets"))
                                   .elementsByTagName(QStringLiteral("presentation"));
    for (int i = 0; i < presElems.count(); ++i) {
        QString pPath = m_fileInfo.path() + QLatin1Char('/')
                + presElems.at(i).toElement().attribute(QStringLiteral("src"));
        if (pPath != doc->GetDocumentPath() && groupExistsInUip(pPath, group)) {
            inUseIdx = i;
            break;
        }
    }

    if (inUseIdx != -1) {
        QMessageBox box;
        box.setWindowTitle(tr("Group tags in use"));
        box.setText(tr("Some tags in the Group '%1' are in use in the project, are you sure you"
                       " want to delete the group?").arg(group));
        box.setIcon(QMessageBox::Warning);
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        box.setButtonText(QMessageBox::Yes, QStringLiteral("Delete"));
        switch (box.exec()) {
        case QMessageBox::Yes:
            // delete the group from all uips that use it
            for (int i = inUseIdx; i < presElems.count(); ++i) {
                QString pPath = m_fileInfo.path() + QLatin1Char('/')
                        + presElems.at(i).toElement().attribute(QStringLiteral("src"));
                if (pPath != doc->GetDocumentPath())
                    deleteGroupFromUip(pPath, group);
            }
            break;

        default:
            // abort deletion
            return;
        }
    }

    // delete the group from current uip, if exists
    deleteGroupFromUip(doc->GetDocumentPath(), group);

    // delete the group from the property (if set)
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    const auto layers = doc->getLayers();
    auto property = bridge->GetLayer().m_variants;
    for (auto layer : layers) {
        qt3dsdm::SValue sValue;
        if (propertySystem->GetInstancePropertyValue(layer, property, sValue)) {
            QString propVal = qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->toQString();
            if (propVal.contains(QStringLiteral("%1:").arg(group))) {
                // property has the deleted group, need to update it, else the deleted group
                // will be saved the uip if the user saves the presentation.
                QRegExp rgx(QStringLiteral("%1:\\w*,*|,%1:\\w*").arg(group));
                propVal.replace(rgx, {});
                qt3dsdm::SValue sVal = std::make_shared<qt3dsdm::CDataStr>(
                                       Q3DStudio::CString::fromQString(propVal));
                propertySystem->SetInstancePropertyValue(layer, property, sVal);
            }
        }
    }

    // update and save the uia
    QDomElement variantsElem = domDoc.documentElement()
                               .firstChildElement(QStringLiteral("variants"));
    QDomNodeList groupsElems = variantsElem.elementsByTagName(QStringLiteral("variantgroup"));

    bool deleted = false;
    for (int i = 0; i < groupsElems.count(); ++i) {
        QDomElement gElem = groupsElems.at(i).toElement();
        if (gElem.attribute(QStringLiteral("id")) == group) {
            variantsElem.removeChild(gElem);
            StudioUtils::commitDomDocumentSave(file, domDoc);
            deleted = true;
            break;
        }
    }

    // update m_variantsDef
    m_variantsDef.remove(group);
}

void ProjectFile::changeVariantGroupColor(const QString &group, const QString &newColor)
{
    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    // update and save the uia
    QDomNodeList groupsElems = domDoc.documentElement()
                                     .firstChildElement(QStringLiteral("variants"))
                                     .elementsByTagName(QStringLiteral("variantgroup"));

    for (int i = 0; i < groupsElems.count(); ++i) {
        QDomElement gElem = groupsElems.at(i).toElement();
        if (gElem.attribute(QStringLiteral("id")) == group) {
            gElem.setAttribute(QStringLiteral("color"), newColor);
            StudioUtils::commitDomDocumentSave(file, domDoc);
            break;
        }
    }

    // update m_variantsDef
    m_variantsDef[group].m_color = newColor;
}

bool ProjectFile::tagExistsInUip(const QString &src, const QString &group, const QString &tag) const
{
    QFile file(src);
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        qWarning() << file.errorString();
        return false;
    }

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (!reader.atEnd()) {
        if (reader.readNextStartElement()) {
            if (reader.name() == QLatin1String("Layer")
                && reader.attributes().hasAttribute(QLatin1String("variants"))) {
                QStringRef v = reader.attributes().value(QLatin1String("variants"));
                if (v.contains(group + QLatin1Char(':') + tag))
                    return true;
            } else if (reader.name() == QLatin1String("Logic")) {
                break;
            }
        }
    }

    return false;
}

bool ProjectFile::groupExistsInUip(const QString &src, const QString &group) const
{
    QFile file(src);
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        qWarning() << file.errorString();
        return false;
    }

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    while (!reader.atEnd()) {
        if (reader.readNextStartElement()) {
            if (reader.name() == QLatin1String("Layer")
                && reader.attributes().hasAttribute(QLatin1String("variants"))) {
                QStringRef v = reader.attributes().value(QLatin1String("variants"));
                if (v.contains(group + QLatin1Char(':')))
                    return true;
            } else if (reader.name() == QLatin1String("Logic")) {
                break;
            }
        }
    }

    return false;
}

// renames a tag (if exists) in all layers in a uip file
void ProjectFile::renameTagInUip(const QString &src, const QString &group, const QString &tag,
                                 const QString &newName)
{
    QDomDocument domDoc;
    QSaveFile file(src);
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomNodeList layerElems = domDoc.documentElement()
                                    .elementsByTagName(QStringLiteral("Layer"));
    bool needSave = false;
    for (int i = 0; i < layerElems.count(); ++i) {
        QDomElement lElem = layerElems.at(i).toElement();
        if (lElem.hasAttribute(QStringLiteral("variants"))) {
            QStringList tagPairs = lElem.attribute(QStringLiteral("variants"))
                                   .split(QLatin1Char(','));
            QString tagFrom = group + QLatin1Char(':') + tag;
            QString tagTo = group + QLatin1Char(':') + newName;

            if (tagPairs.contains(tagFrom)) {
                tagPairs.replaceInStrings(tagFrom, tagTo);
                lElem.setAttribute(QStringLiteral("variants"), tagPairs.join(QLatin1Char(',')));
                needSave = true;
            }
        }
    }

    if (needSave)
        StudioUtils::commitDomDocumentSave(file, domDoc);
}

void ProjectFile::renameGroupInUip(const QString &src, const QString &group, const QString &newName)
{
    QDomDocument domDoc;
    QSaveFile file(src);
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomNodeList layerElems = domDoc.documentElement()
                                    .elementsByTagName(QStringLiteral("Layer"));
    bool needSave = false;
    for (int i = 0; i < layerElems.count(); ++i) {
        QDomElement lElem = layerElems.at(i).toElement();
        if (lElem.hasAttribute(QStringLiteral("variants"))) {
            QString variants = lElem.attribute(QStringLiteral("variants"));
            if (variants.contains(group + QLatin1Char(':'))) {
                variants.replace(group + QLatin1Char(':'), newName + QLatin1Char(':'));
                lElem.setAttribute(QStringLiteral("variants"), variants);
                needSave = true;
            }
        }
    }

    if (needSave)
        StudioUtils::commitDomDocumentSave(file, domDoc);
}

// deletes a tag (if exists) from all layers in a uip file
void ProjectFile::deleteTagFromUip(const QString &src, const QString &group, const QString &tag)
{
    QDomDocument domDoc;
    QSaveFile file(src);
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomNodeList layerElems = domDoc.documentElement()
                                    .elementsByTagName(QStringLiteral("Layer"));
    bool needSave = false;
    for (int i = 0; i < layerElems.count(); ++i) {
        QDomElement lElem = layerElems.at(i).toElement();
        if (lElem.hasAttribute(QStringLiteral("variants"))) {
            QStringList tagPairs = lElem.attribute(QStringLiteral("variants"))
                                                                        .split(QLatin1Char(','));
            QString tagPair = group + QLatin1Char(':') + tag;
            if (tagPairs.contains(tagPair)) {
                tagPairs.removeOne(tagPair);
                lElem.setAttribute(QStringLiteral("variants"), tagPairs.join(QLatin1Char(',')));
                needSave = true;
            }
        }
    }

    if (needSave)
        StudioUtils::commitDomDocumentSave(file, domDoc);
}

// deletes a group (if exists) from all layers in a uip file
void ProjectFile::deleteGroupFromUip(const QString &src, const QString &group)
{
    QDomDocument domDoc;
    QSaveFile file(src);
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    QDomNodeList layerElems = domDoc.documentElement()
                                    .elementsByTagName(QStringLiteral("Layer"));
    bool needSave = false;
    QRegExp rgx(group + ":\\w*,*|," + group + ":\\w*");
    for (int i = 0; i < layerElems.count(); ++i) {
        QDomElement lElem = layerElems.at(i).toElement();
        if (lElem.hasAttribute(QStringLiteral("variants"))) {
            QString val = lElem.attribute(QStringLiteral("variants"));
            if (rgx.indexIn(val) != -1) {
                val.replace(rgx, "");
                lElem.setAttribute(QStringLiteral("variants"), val);
                needSave = true;
            }
        }
    }

    if (needSave)
        StudioUtils::commitDomDocumentSave(file, domDoc);
}

bool ProjectFile::isVariantGroupUnique(const QString &group) const
{
    return !m_variantsDef.contains(group);
}

bool ProjectFile::isVariantTagUnique(const QString &group, const QString &tag) const
{
    if (!m_variantsDef.contains(group))
        return true;

    return !m_variantsDef[group].m_tags.contains(tag);
}

void ProjectFile::deleteVariantTag(const QString &group, const QString &tag)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    QDomDocument domDoc;
    QSaveFile file(getProjectFilePath());
    if (!StudioUtils::openDomDocumentSave(file, domDoc))
        return;

    // check if tag is in use in other presentations in the porject
    int inUseIdx = -1; // list of presentations that has the tag in use
    QDomNodeList presElems = domDoc.documentElement()
                                   .firstChildElement(QStringLiteral("assets"))
                                   .elementsByTagName(QStringLiteral("presentation"));
    for (int i = 0; i < presElems.count(); ++i) {
        QString pPath = m_fileInfo.path() + QLatin1Char('/')
                + presElems.at(i).toElement().attribute(QStringLiteral("src"));
        if (pPath != doc->GetDocumentPath()
                && tagExistsInUip(pPath, group, tag)) {
            inUseIdx = i;
            break;
        }
    }

    if (inUseIdx != -1) {
        QMessageBox box;
        box.setWindowTitle(tr("Tag in use"));
        box.setText(tr("The tag '%1' is in use in another presentation, are you sure you want to"
                       " delete it?").arg(tag));
        box.setIcon(QMessageBox::Warning);
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        box.setButtonText(QMessageBox::Yes, QStringLiteral("Delete"));
        switch (box.exec()) {
        case QMessageBox::Yes:
            // delete the tag from all uips that use it
            for (int i = inUseIdx; i < presElems.count(); ++i) {
                QString pPath = m_fileInfo.path() + QLatin1Char('/')
                        + presElems.at(i).toElement().attribute(QStringLiteral("src"));
                if (pPath != doc->GetDocumentPath())
                    deleteTagFromUip(pPath, group, tag);
            }
            break;

        default:
            // abort deletion
            return;
        }
    }

    // delete the tag from current doc, if exists
    deleteTagFromUip(doc->GetDocumentPath(), group, tag);

    QDomNodeList groupsElems = domDoc.documentElement()
                                     .firstChildElement(QStringLiteral("variants"))
                                     .elementsByTagName(QStringLiteral("variantgroup"));

    // delete the tag from the property (if set)
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    const auto layers = doc->getLayers();
    auto property = bridge->GetLayer().m_variants;
    for (auto layer : layers) {
        qt3dsdm::SValue sValue;
        if (propertySystem->GetInstancePropertyValue(layer, property, sValue)) {
            QString propVal = QString::fromWCharArray(qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)
                                                      ->GetData());
            if (propVal.contains(QStringLiteral("%1:%2").arg(group).arg(tag))) {
                // property has the deleted tag, need to update it, else the deleted tag will be
                // saved in the uip if the user saves the presentation.
                QRegExp rgx(QStringLiteral("%1:%2,*|,%1:%2").arg(group).arg(tag));
                propVal.replace(rgx, {});
                qt3dsdm::SValue sVal = std::make_shared<qt3dsdm::CDataStr>(
                                       Q3DStudio::CString::fromQString(propVal));
                propertySystem->SetInstancePropertyValue(layer, property, sVal);
            }
        }
    }

    // update and save the uia
    bool deleted = false;
     for (int i = 0; i < groupsElems.count(); ++i) {
        QDomElement gElem = groupsElems.at(i).toElement();
        if (gElem.attribute(QStringLiteral("id")) == group) {
            QDomNodeList tagsElems = gElem.childNodes();
            for (int j = 0; j < tagsElems.count(); ++j) {
                QDomElement tElem = tagsElems.at(j).toElement();
                if (tElem.attribute(QStringLiteral("id")) == tag) {
                    gElem.removeChild(tElem);
                    StudioUtils::commitDomDocumentSave(file, domDoc);
                    deleted = true;
                    break;
                }
            }
            if (deleted)
                break;
        }
    }

    // update m_variantsDef
     m_variantsDef[group].m_tags.removeOne(tag);
}
