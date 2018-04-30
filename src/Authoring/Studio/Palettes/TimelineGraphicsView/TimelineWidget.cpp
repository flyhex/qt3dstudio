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
#include "RowMover.h"
#include "KeyframeManager.h"
#include "RowTree.h"
#include "Keyframe.h"
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
    m_viewTimelineHeader->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    m_viewTimelineContent->setScene(m_graphicsScene);
    m_viewTimelineContent->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTimelineContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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

    m_navigationBar = new NavigationBar(this);

    auto *layoutRoot = new QVBoxLayout;
    layoutRoot->setContentsMargins(0, 0, 0, 0);
    layoutRoot->setSpacing(0);
    layoutRoot->addWidget(m_navigationBar);
    layoutRoot->addLayout(layoutContent);
    layoutRoot->addWidget(m_toolbar);
    setLayout(layoutRoot);

    g_StudioApp.GetCore()->GetDoc()->SetKeyframesManager(
                static_cast<IKeyframesManager *>(m_graphicsScene->keyframeManager()));

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
        // Note: Slider value starts from TREE_BOUND_W, make start from 0
        int viewportX = std::max(0, value - (int)TimelineConstants::TREE_BOUND_W);
        m_graphicsScene->ruler()->setViewportX(viewportX);
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

    connect(m_graphicsScene->ruler(), &Ruler::durationChanged, this, [this]() {
        m_graphicsScene->updateTimelineLayoutWidth();
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
        std::bind(&TimelineWidget::onActiveSlide, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)));

    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();

    m_connections.push_back(theDispatch->ConnectSelectionChange(
        std::bind(&TimelineWidget::onSelectionChange, this, std::placeholders::_1)));

    // object created/deleted
    m_connections.push_back(theSignalProvider->ConnectInstanceCreated(
        std::bind(&TimelineWidget::onAssetCreated, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectInstanceDeleted(
        std::bind(&TimelineWidget::onAssetDeleted, this, std::placeholders::_1)));

    // animation created/deleted
    m_connections.push_back(theSignalProvider->ConnectAnimationCreated(
        std::bind(&TimelineWidget::onAnimationCreated, this,
             std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectAnimationDeleted(
        std::bind(&TimelineWidget::onAnimationDeleted, this,
             std::placeholders::_2, std::placeholders::_3)));

    // keyframe added/deleted
    m_connections.push_back(theSignalProvider->ConnectKeyframeInserted(
        std::bind(&TimelineWidget::onKeyframeInserted, this,
             std::placeholders::_1, std::placeholders::_2)));
    m_connections.push_back(theSignalProvider->ConnectKeyframeErased(
        std::bind(&TimelineWidget::onKeyframeDeleted, this,
             std::placeholders::_1, std::placeholders::_2)));
    m_connections.push_back(theSignalProvider->ConnectKeyframeUpdated(
        std::bind(&TimelineWidget::onKeyframeUpdated, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectInstancePropertyValue(
        std::bind(&TimelineWidget::onPropertyChanged, this,
             std::placeholders::_1, std::placeholders::_2)));

    // action created/deleted
    m_connections.push_back(theSignalProvider->ConnectActionCreated(
        std::bind(&TimelineWidget::onActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectActionDeleted(
        std::bind(&TimelineWidget::onActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

    // object add, remove, move
    Q3DStudio::CGraph &theGraph(*g_StudioApp.GetCore()->GetDoc()->GetAssetGraph());
    m_connections.push_back(theGraph.ConnectChildAdded(
        std::bind(&TimelineWidget::onChildAdded, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theGraph.ConnectChildRemoved(
        std::bind(&TimelineWidget::onChildRemoved, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theGraph.ConnectChildMoved(
        std::bind(&TimelineWidget::onChildMoved, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));

    // Connect toolbar play/stop now when m_pMainWnd exists
    connect(g_StudioApp.m_pMainWnd, &CMainFrame::playStateChanged,
            m_toolbar, &TimelineToolbar::updatePlayButtonState);

    // Clear active slide
    m_activeSlide = qt3dsdm::Qt3DSDMSlideHandle();
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

void TimelineWidget::onActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
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
    m_graphicsScene->updateSnapSteps();
    m_handlesMap.clear();
    insertToHandlesMapRecursive(m_binding);
    m_navigationBar->updateNavigationItems(m_translationManager->GetBreadCrumbProvider());
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

void TimelineWidget::onSelectionChange(Q3DStudio::SSelectedValue inNewSelectable)
{
    qt3dsdm::TInstanceHandleList theInstances = inNewSelectable.GetSelectedInstances();
    if (theInstances.size() > 0) {
        for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
            if (g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->IsInstance(theInstance)) {
                Qt3DSDMTimelineItemBinding *selectedBinding = getBindingForHandle(theInstance,
                                                                                  m_binding);
                if (selectedBinding)
                    m_graphicsScene->rowManager()->selectRow(selectedBinding->getRowTree());
                else
                    m_graphicsScene->rowManager()->clearSelection();
            }
        }
    } else {
        m_graphicsScene->rowManager()->clearSelection();
    }

    // Mahmoud_TODO: Expand the tree so the selection is visible
}

void TimelineWidget::onAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    CClientDataModelBridge *theDataModelBridge = g_StudioApp.GetCore()->GetDoc()
                                                 ->GetStudioSystem()->GetClientDataModelBridge();

    if (theDataModelBridge->IsSceneGraphInstance(inInstance)) {
        Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(inInstance, m_binding);
        if (binding) {
            Qt3DSDMTimelineItemBinding *bindingParent = getBindingForHandle(theDataModelBridge
                                                        ->GetParentInstance(inInstance), m_binding);
            RowTree *newRow = m_graphicsScene->rowManager()
                              ->createRowFromBinding(binding, bindingParent->getRowTree());

            m_handlesMap.insert(std::make_pair(inInstance, newRow));
        }
    }
}

void TimelineWidget::onAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    THandleMap::const_iterator it = m_handlesMap.find(inInstance);

    if (it != m_handlesMap.end()) { // scene object exists
        m_graphicsScene->rowManager()->deleteRow(it->second);
        m_handlesMap.erase(it);
    }
}

void TimelineWidget::onAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(parentInstance, m_binding);
    if (binding) {
        ITimelineItemProperty *propBinding = binding->GetPropertyBinding(property);

        // create the binding if doesn't exist
        if (!propBinding) {
            propBinding = binding->GetOrCreatePropertyBinding(property);

            // create the property UI row
            RowTree *propRow = m_graphicsScene->rowManager()
            ->getOrCreatePropertyRow(binding->getRowTree(), propBinding->GetName().toQString(),
                                     binding->getAnimatedPropertyIndex(property));

            // connect the row and binding
            propBinding->setRowTree(propRow);
            propRow->setPropBinding(propBinding);

            // add keyframes
            for (int i = 0; i < propBinding->GetKeyframeCount(); i++) {
                IKeyframe *kf = propBinding->GetKeyframeByIndex(i);
                Keyframe *kfUI = m_graphicsScene->keyframeManager()->insertKeyframe(
                            propRow->rowTimeline(), static_cast<double>(kf->GetTime()) * .001, false)
                            .at(0);

                kf->setUI(kfUI);
                kfUI->binding = static_cast<Qt3DSDMTimelineKeyframe *>(kf);
            }

            propRow->update();
        }
    }
}

void TimelineWidget::onAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(parentInstance, m_binding);
    if (binding) {
        ITimelineItemProperty *propBinding = binding->GetPropertyBinding(property);

        if (propBinding) {
            m_graphicsScene->rowManager()->deleteRow(propBinding->getRowTree());

            binding->RemovePropertyRow(property);
        }
    }
}

void TimelineWidget::onKeyframeInserted(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                                        qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe)
{
    refreshKeyframe(inAnimation, inKeyframe, ETimelineKeyframeTransaction_Add);
}

void TimelineWidget::onKeyframeDeleted(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                                       qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe)
{
    refreshKeyframe(inAnimation, inKeyframe, ETimelineKeyframeTransaction_Delete);
}

void TimelineWidget::onKeyframeUpdated(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe)
{
    qt3dsdm::IAnimationCore *theAnimationCore =
            g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetAnimationCore();
    if (theAnimationCore->KeyframeValid(inKeyframe)) {
        qt3dsdm::Qt3DSDMAnimationHandle theAnimationHandle =
            theAnimationCore->GetAnimationForKeyframe(inKeyframe);
        refreshKeyframe(theAnimationHandle, inKeyframe, ETimelineKeyframeTransaction_Update);
    }
}

void TimelineWidget::refreshKeyframe(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                                     qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                                     ETimelineKeyframeTransaction inTransaction)
{
    qt3dsdm::CStudioSystem *studioSystem = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    if (studioSystem->GetAnimationCore()->AnimationValid(inAnimation)) {
        qt3dsdm::SAnimationInfo theAnimationInfo =
            studioSystem->GetAnimationCore()->GetAnimationInfo(inAnimation);
        Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(theAnimationInfo.m_Instance,
                                                                  m_binding);
        if (binding != nullptr) {
            binding->RefreshPropertyKeyframe(theAnimationInfo.m_Property, inKeyframe,
                                             inTransaction);

            binding->getRowTree()->rowTimeline()->updateKeyframesFromBinding(
                        theAnimationInfo.m_Property);
        }
    }
}

void TimelineWidget::onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                       qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(inInstance, m_binding);

    if (binding != nullptr) {
        binding->OnPropertyChanged(inProperty);

        if (binding->getRowTree() != nullptr)
            binding->getRowTree()->rowTimeline()->updateDurationFromBinding();
    }
}

void TimelineWidget::onActionEvent(qt3dsdm::Qt3DSDMActionHandle inAction,
                                   qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                   qt3dsdm::Qt3DSDMInstanceHandle inOwner)
{
    Q_UNUSED(inAction)
    Q_UNUSED(inSlide)
    Q_UNUSED(inOwner)

    // Mahmoud_TODO: implement?
}

void TimelineWidget::onChildAdded(int inParent, int inChild, long inIndex)
{
    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(inChild, m_binding);
    Qt3DSDMTimelineItemBinding *bindingParent = getBindingForHandle(inParent, m_binding);

    if (binding && bindingParent) {
        RowTree *row = binding->getRowTree();
        RowTree *rowParent = bindingParent->getRowTree();

        if (row && rowParent)
            rowParent->addChildAt(row, inIndex);
    }
}

void TimelineWidget::onChildRemoved(int inParent, int inChild, long inIndex)
{
    // Mahmoud_TODO: implement?
}

void TimelineWidget::onChildMoved(int inParent, int inChild, long inOldIndex,
                                  long inNewIndex)
{
    Q_UNUSED(inOldIndex)

    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(inChild, m_binding);
    Qt3DSDMTimelineItemBinding *bindingParent = getBindingForHandle(inParent, m_binding);

    if (binding && bindingParent) {
        RowTree *row = binding->getRowTree();
        RowTree *rowParent = bindingParent->getRowTree();
        rowParent->addChildAt(row, inNewIndex);
    }
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

bool TimelineWidget::hasSelectedKeyframes() const
{
    return m_graphicsScene->keyframeManager()->hasSelectedKeyframes();
}

RowTree *TimelineWidget::selectedRow() const
{
    return m_graphicsScene->rowManager()->selectedRow();
}
