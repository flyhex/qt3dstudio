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

#include "TimelineWidget.h"
#include "TimelineGraphicsScene.h"
#include "TimelineConstants.h"
#include "TimelineToolbar.h"
#include "RowManager.h"
#include "KeyframeManager.h"
#include "RowTree.h"
#include "PlayHead.h"
#include "Ruler.h"
#include "TimelineSplitter.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "Dispatch.h"
#include "MainFrm.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"
#include "ClientDataModelBridge.h"
#include "Bindings/TimelineTranslationManager.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemProperty.h"
#include "TimeEditDlg.h"

#include <QtGui/qevent.h>
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qlabel.h>

// Mahmoud_TODO: debug func, to be removed
void printBinding(ITimelineItemBinding *binding, QString padding = " ")
{
    qDebug().noquote().nospace()
            << "\x1b[42m \x1b[1m" << __FUNCTION__
            << padding
            << binding->GetTimelineItem()->GetName().toQString()
            << " (" << static_cast<Qt3DSDMTimelineItemBinding *>(binding)->GetInstance() << ")"
            << "\x1b[m";

    for (int i = 0; i < binding->GetPropertyCount(); i++) {
        ITimelineItemProperty *property = binding->GetProperty(i);
        qDebug().noquote().nospace()
            << "\x1b[42m \x1b[1m" << __FUNCTION__
            << padding
            << "[" << property->GetName().toQString() << "]"
            << " (" << static_cast<Qt3DSDMTimelineItemProperty*>(property)->getPropertyHandle() << ")"
            << "\x1b[m";

        for (int j = 0; j < property->GetKeyframeCount(); j++) {
            IKeyframe *kf = property->GetKeyframeByIndex(j);
            qDebug().noquote().nospace()
                << "\x1b[42m \x1b[1m" << __FUNCTION__
                << padding
                << "  {KF: " << kf->GetTime() << ", selected: " << kf->IsSelected() << "}"
                << "\x1b[m";
        }
    }
    padding = padding.append("-");

    // create child rows recursively
    for (int i = 0; i < binding->GetChildrenCount(); i++)
        printBinding(binding->GetChild(i), padding);
}

// Mahmoud_TODO: debug func, to be removed
void printHandlesMap(std::map<qt3dsdm::Qt3DSDMInstanceHandle, RowTree *> theBindingMap)
{
    for (auto& kv : theBindingMap)
        qDebug().noquote().nospace()
                 << "\x1b[42m \x1b[1m" << __FUNCTION__
                 << ", k=" << kv.first
                 << ", v=" << (kv.second == nullptr ? "--" : kv.second->label())
                 << "\x1b[m";
}

class Eventfilter : public QObject
{
public:
    Eventfilter(QObject *parent) : QObject(parent) {}

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::Wheel) {
            event->accept();
            return true;
        }

        return false;
    }
};

TimelineWidget::TimelineWidget(QWidget *parent)
    : QWidget()
    , m_toolbar(new TimelineToolbar())
    , m_viewTreeHeader(new TreeHeaderView(this))
    , m_viewTreeContent(new QGraphicsView(this))
    , m_viewTimelineHeader(new QGraphicsView(this))
    , m_viewTimelineContent(new QGraphicsView(this))
    , m_graphicsScene(new TimelineGraphicsScene(this))
{
    m_translationManager = new CTimelineTranslationManager();

    setWindowTitle(tr("Timeline", "Title of timeline view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_viewTreeHeader->setScene(m_graphicsScene);
    m_viewTreeHeader->setFixedHeight(TimelineConstants::ROW_H);
    m_viewTreeHeader->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTreeHeader->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeHeader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeHeader->viewport()->installEventFilter(new Eventfilter(this));
    m_viewTreeHeader->viewport()->setFocusPolicy(Qt::NoFocus);
    m_viewTreeHeader->setFixedWidth(TimelineConstants::TREE_DEFAULT_W);

    m_viewTreeContent->setScene(m_graphicsScene);
    m_viewTreeContent->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTreeContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_viewTreeContent->horizontalScrollBar()->hide();
    m_viewTreeContent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeContent->setFixedWidth(TimelineConstants::TREE_DEFAULT_W);

    m_viewTimelineHeader->setScene(m_graphicsScene);
    m_viewTimelineHeader->setFixedHeight(TimelineConstants::ROW_H);
    m_viewTimelineHeader->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTimelineHeader->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTimelineHeader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_viewTimelineHeader->verticalScrollBar()->hide();
    m_viewTimelineHeader->viewport()->installEventFilter(new Eventfilter(this));
    m_viewTimelineHeader->viewport()->setFocusPolicy(Qt::NoFocus);

    m_viewTimelineContent->setScene(m_graphicsScene);
    m_viewTimelineContent->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTimelineContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_viewTimelineContent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_viewTimelineContent->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    auto *layoutTree = new QVBoxLayout;
    layoutTree->setContentsMargins(QMargins(0, 0, 0, 0));
    layoutTree->addWidget(m_viewTreeHeader);
    layoutTree->addWidget(m_viewTreeContent);

    m_splitter = new TimelineSplitter(this);

    auto *layoutTimeline = new QVBoxLayout;
    layoutTimeline->setContentsMargins(QMargins(0, 0, 0, 0));
    layoutTimeline->addWidget(m_viewTimelineHeader);
    layoutTimeline->addWidget(m_viewTimelineContent);

    auto *layoutContent = new QHBoxLayout;
    layoutContent->setContentsMargins(QMargins(0, 0, 0, 10));
    layoutContent->addLayout(layoutTree);
    layoutContent->addWidget(m_splitter);
    layoutContent->addLayout(layoutTimeline);

    auto *layoutRoot = new QVBoxLayout;
    layoutRoot->setContentsMargins(0, 0, 0, 0);
    layoutRoot->setSpacing(0);
    layoutRoot->addLayout(layoutContent);
    layoutRoot->addWidget(m_toolbar);
    setLayout(layoutRoot);

    // connect graphics scene geometryChanged
    connect(m_graphicsScene->widgetRoot(), &QGraphicsWidget::geometryChanged, this, [this]() {
        const QRectF rect = m_graphicsScene->widgetRoot()->rect();

        m_viewTreeContent->setSceneRect(QRectF(0, TimelineConstants::ROW_H,
                                               TimelineConstants::TREE_MAX_W, rect.height()));

        m_viewTimelineHeader->setSceneRect(QRectF(TimelineConstants::TREE_BOUND_W, 0,
                                                  rect.width() - TimelineConstants::TREE_BOUND_W,
                                                  TimelineConstants::ROW_H));

        m_viewTimelineContent->setSceneRect(QRectF(TimelineConstants::TREE_BOUND_W,
                                                   TimelineConstants::ROW_H,
                                                   rect.width() - TimelineConstants::TREE_BOUND_W,
                                                   rect.height()));

        m_graphicsScene->playHead()->setHeight(m_graphicsScene->widgetRoot()->geometry().height());
    });

    // connect timeline and ruler horizontalScrollBars
    connect(m_viewTimelineContent->horizontalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTimelineHeader->horizontalScrollBar()->setValue(value);
    });

    // connect timeline and tree verticalScrollBars
    connect(m_viewTimelineContent->verticalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTreeContent->verticalScrollBar()->setValue(value);
    });

    // connect tree and timeline verticalScrollBars
    connect(m_viewTreeContent->verticalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTimelineContent->verticalScrollBar()->setValue(value);
    });

    // connect tree and tree header horizontalScrollBars
    connect(m_viewTreeContent->horizontalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTreeHeader->horizontalScrollBar()->setValue(value);
    });

    connect(m_toolbar, &TimelineToolbar::newLayerTriggered, this, [this]() {
        // Mahmoud_TODO: debug code, remove
//        printHandlesMap(m_handlesMap);
        using namespace Q3DStudio;

        CDoc *doc = g_StudioApp.GetCore()->GetDoc();
        CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();

        // If active instance is component, just bail as we can't add layers to components
        qt3dsdm::Qt3DSDMInstanceHandle rootInstance = doc->GetActiveRootInstance();
        if (bridge->GetObjectType(rootInstance) == OBJTYPE_COMPONENT)
            return;

        qt3dsdm::Qt3DSDMSlideHandle slide = doc->GetActiveSlide();
        qt3dsdm::Qt3DSDMInstanceHandle layer = doc->GetActiveLayer();

        SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Add Layer"))
            ->CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Layer, layer, slide,
                                       DocumentEditorInsertType::PreviousSibling,
                                       CPt(), PRIMITIVETYPE_UNKNOWN, -1);
    });

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    connect(m_toolbar, &TimelineToolbar::deleteLayerTriggered, doc, &CDoc::DeleteSelectedObject);

    connect(m_toolbar, &TimelineToolbar::gotoTimeTriggered, this, [this]() {
        CDoc *doc = g_StudioApp.GetCore()->GetDoc();
        CTimeEditDlg timeEditDlg;
        timeEditDlg.showDialog(doc->GetCurrentViewTime(), doc, PLAYHEAD);
    });

    connect(m_toolbar, &TimelineToolbar::firstFrameTriggered, this, [this]() {
        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(0);
    });

    connect(m_toolbar, &TimelineToolbar::stopTriggered, this, [this]() {
        g_StudioApp.PlaybackStopNoRestore();
    });

    connect(m_toolbar, &TimelineToolbar::playTriggered, this, [this]() {
        g_StudioApp.PlaybackPlay();
    });

    connect(m_toolbar, &TimelineToolbar::lastFrameTriggered, this, [this]() {
        double dur = m_graphicsScene->ruler()->duration() * 1000;
        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(dur);
    });

    connect(m_toolbar, &TimelineToolbar::timelineScaleChanged, this, [this](int scale) {
        m_graphicsScene->setTimelineScale(scale);
    });

    // data model listeners
    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);
    g_StudioApp.GetCore()->GetDispatch()->AddClientPlayChangeListener(this);
}

void TimelineWidget::OnNewPresentation()
{
    // Register callbacks
    qt3dsdm::IStudioFullSystemSignalProvider *theSignalProvider =
        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystemSignalProvider();

    m_connections.push_back(theSignalProvider->ConnectActiveSlide(
        std::bind(&TimelineWidget::OnActiveSlide, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)));

    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();

    m_connections.push_back(theDispatch->ConnectSelectionChange(
        std::bind(&TimelineWidget::OnSelectionChange, this, std::placeholders::_1)));

    // object created/deleted
    m_connections.push_back(theSignalProvider->ConnectInstanceCreated(
        std::bind(&TimelineWidget::OnAssetCreated, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectInstanceDeleted(
        std::bind(&TimelineWidget::OnAssetDeleted, this, std::placeholders::_1)));

    // animation created/deleted
    m_connections.push_back(theSignalProvider->ConnectAnimationCreated(
        std::bind(&TimelineWidget::OnAnimationCreated, this,
             std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectAnimationDeleted(
        std::bind(&TimelineWidget::OnAnimationDeleted, this,
             std::placeholders::_2, std::placeholders::_3)));

    // action created/deleted
    m_connections.push_back(theSignalProvider->ConnectActionCreated(
        std::bind(&TimelineWidget::OnActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectActionDeleted(
        std::bind(&TimelineWidget::OnActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

    // Connect toolbar play/stop now when m_pMainWnd exists
    connect(g_StudioApp.m_pMainWnd, &CMainFrame::playStateChanged,
            m_toolbar, &TimelineToolbar::updatePlayButtonState);

}

void TimelineWidget::OnClosingPresentation()
{
    m_connections.clear();
}

void TimelineWidget::OnTimeChanged(long inTime)
{
    m_graphicsScene->playHead()->setTime(inTime * .001);
    m_toolbar->setTime(inTime);
}

void TimelineWidget::OnActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                                   const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    Q_UNUSED(inMaster);
    Q_UNUSED(inIndex);

    if (m_activeSlide == inSlide)
        return;

    m_translationManager->Clear();
    m_activeSlide = inSlide;

    auto *theSlideSystem = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetSlideSystem();
    auto theSlideInstance = theSlideSystem->GetSlideInstance(inSlide);

    m_binding = static_cast<Qt3DSDMTimelineItemBinding *>(
                m_translationManager->GetOrCreate(theSlideInstance));

    m_graphicsScene->rowManager()->recreateRowsFromBinding(m_binding);
    m_handlesMap.clear();
    insertToHandlesMapRecursive(m_binding);
}

void TimelineWidget::insertToHandlesMapRecursive(Qt3DSDMTimelineItemBinding *binding)
{
    if (binding->GetObjectType() != OBJTYPE_MATERIAL) {
        m_handlesMap.insert(std::make_pair(binding->GetInstance(), binding->getRowTree()));

       for (int i = 0; i < binding->GetChildrenCount(); i++) {
           insertToHandlesMapRecursive(
                       static_cast<Qt3DSDMTimelineItemBinding *>(binding->GetChild(i)));
       }
    }
}

void TimelineWidget::OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable)
{
    qt3dsdm::TInstanceHandleList theInstances = inNewSelectable.GetSelectedInstances();
    for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
        if (g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->IsInstance(theInstance)) {
            // Mahmoud_TODO: remove debug
            qDebug() << "\x1b[42m \x1b[1m" << __FUNCTION__
                     << theInstance
                     << "\x1b[m";
            Qt3DSDMTimelineItemBinding *selectedBinding = getBindingForHandle(theInstance,
                                                                              m_binding);
            m_graphicsScene->rowManager()->selectRow(selectedBinding->getRowTree());
        }
    }

    // Mahmoud_TODO: Expand the tree so the selection is visible
}

void TimelineWidget::OnAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    CClientDataModelBridge *theDataModelBridge = g_StudioApp.GetCore()->GetDoc()
                                                 ->GetStudioSystem()->GetClientDataModelBridge();

    if (theDataModelBridge->IsSceneGraphInstance(inInstance)) {
        Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(inInstance, m_binding);
        Qt3DSDMTimelineItemBinding *bindingParent = getBindingForHandle(theDataModelBridge
                                                    ->GetParentInstance(inInstance), m_binding);
        // Mahmoud_TODO: remove debug
        qDebug() << "\x1b[42m \x1b[1m" << __FUNCTION__
                 << ", binding=" << binding->GetName().toQString()
                 << ", bindingParent=" << bindingParent->GetName().toQString()
                 << "\x1b[m";
        RowTree *newRow = m_graphicsScene->rowManager()
                          ->createRowFromBinding(binding, bindingParent->getRowTree());

        if (binding->GetObjectType() == OBJTYPE_LAYER)
            m_graphicsScene->rowManager()->syncRowPositionWithBinding(newRow, bindingParent);

        m_handlesMap.insert(std::make_pair(inInstance, newRow));
    }
}

void TimelineWidget::OnAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    THandleMap::const_iterator it = m_handlesMap.find(inInstance);

    if (it != m_handlesMap.end()) { // scene object exists
        m_graphicsScene->rowManager()->deleteRow(it->second);
        m_handlesMap.erase(it);
    }
}

void TimelineWidget::OnAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(parentInstance, m_binding);
    ITimelineItemProperty *propBinding = binding->GetPropertyBinding(property);

    // create the binding if doesn't exist
    if (propBinding == nullptr) {
        propBinding = binding->GetOrCreatePropertyBinding(property);

        // create the property UI row
        RowTree *propRow = m_graphicsScene->rowManager()
            ->getOrCreatePropertyRow(binding->getRowTree(), propBinding->GetName().toQString());

        // connect the row and binding
        propBinding->setRowTree(propRow);
        propRow->setPropBinding(propBinding);

        // add keyframes
        for (int i = 0; i < propBinding->GetKeyframeCount(); i++) {
            IKeyframe *kf = propBinding->GetKeyframeByIndex(i);
            m_graphicsScene->keyframeManager()->insertKeyframe(propRow->rowTimeline(),
                                        static_cast<double>(kf->GetTime()) * .001, 0, false);
        }

        propRow->update();
    }

    // make sure the property rows are in the same order as in the binding
    m_graphicsScene->rowManager()->reorderPropertiesFromBinding(binding);
}

void TimelineWidget::OnAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(parentInstance, m_binding);
    if (binding != nullptr) {
        ITimelineItemProperty *propBinding = binding->GetPropertyBinding(property);

        if (propBinding != nullptr) {
            m_graphicsScene->rowManager()->deleteRow(propBinding->getRowTree());

                binding->RemovePropertyRow(property);
        }
    }
}

void TimelineWidget::OnActionEvent(qt3dsdm::Qt3DSDMActionHandle inAction,
                                   qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                   qt3dsdm::Qt3DSDMInstanceHandle inOwner)
{
    Q_UNUSED(inAction);
    Q_UNUSED(inSlide);
    Q_UNUSED(inOwner);

    // Mahmoud_TODO: implement
    qDebug() << "\x1b[42m \x1b[1m" << __FUNCTION__ << "\x1b[m";
}

Qt3DSDMTimelineItemBinding *TimelineWidget::getBindingForHandle(int handle,
                                                Qt3DSDMTimelineItemBinding *binding) const
{
    if (binding->GetInstance().GetHandleValue() == handle)
        return binding;

    for (int i = 0; i < binding->GetChildrenCount(); i++) {
        Qt3DSDMTimelineItemBinding *b = getBindingForHandle(handle,
                            static_cast<Qt3DSDMTimelineItemBinding *>(binding->GetChild(i)));

        if (b != nullptr)
            return b;
    }

    return nullptr;
}

void TimelineWidget::mousePressEvent(QMouseEvent *event)
{
    if (childAt(event->pos()) == m_splitter)
        m_splitterPressed = true;
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_splitterPressed) {
        double treeWidth = event->pos().x() - m_splitter->size().width() * .5;
        treeWidth = qBound(TimelineConstants::TREE_MIN_W, treeWidth, TimelineConstants::TREE_MAX_W);

        m_viewTreeHeader->setFixedWidth(treeWidth);
        m_viewTreeContent->setFixedWidth(treeWidth);
        m_graphicsScene->updateTreeWidth(treeWidth);
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_splitterPressed = false;
}

QGraphicsView *TimelineWidget::viewTimelineContent() const
{
    return m_viewTimelineContent;
}

QGraphicsView *TimelineWidget::viewTreeContent() const
{
    return m_viewTreeContent;
}

TimelineToolbar *TimelineWidget::toolbar() const
{
    return m_toolbar;
}
