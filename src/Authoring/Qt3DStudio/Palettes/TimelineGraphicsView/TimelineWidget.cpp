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
#include "Bindings/ITimelineTimebar.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemProperty.h"
#include "Bindings/TimelineBreadCrumbProvider.h"
#include "IDocumentEditor.h"
#include "Control.h"
#include "TimelineDropTarget.h"
#include "StudioPreferences.h"
#include "Dialogs.h"
#include "TimeEnums.h"

#include <QtGui/qevent.h>
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qlabel.h>

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

TimelineWidget::TimelineWidget(const QSize &preferredSize, QWidget *parent)
    : QWidget(parent)
    , m_viewTreeHeader(new TreeHeaderView(this))
    , m_viewTreeContent(new QGraphicsView(this))
    , m_viewTimelineHeader(new QGraphicsView(this))
    , m_viewTimelineContent(new QGraphicsView(this))
    , m_toolbar(new TimelineToolbar())
    , m_graphicsScene(new TimelineGraphicsScene(this))
    , m_preferredSize(preferredSize)
{
    int treeWidth = CStudioPreferences::GetTimelineSplitterLocation();

    // Mahmoud_TODO: CTimelineTranslationManager should be eventually removed or cleaned. Already
    // most of its functionality is implemented in this class
    m_translationManager = new CTimelineTranslationManager();
    m_BreadCrumbProvider = new CTimelineBreadCrumbProvider(g_StudioApp.GetCore()->GetDoc());

    setWindowTitle(tr("Timeline", "Title of timeline view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_viewTreeHeader->setScene(m_graphicsScene);
    m_viewTreeHeader->setFixedHeight(TimelineConstants::ROW_H);
    m_viewTreeHeader->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTreeHeader->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeHeader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeHeader->viewport()->installEventFilter(new Eventfilter(this));
    m_viewTreeHeader->viewport()->setFocusPolicy(Qt::NoFocus);
    m_viewTreeHeader->setFixedWidth(treeWidth);

    m_viewTreeContent->setScene(m_graphicsScene);
    m_viewTreeContent->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTreeContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeContent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_viewTreeContent->setFixedWidth(treeWidth);

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

    setTreeWidth(treeWidth);

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
    layoutContent->setContentsMargins(QMargins(0, 0, 0, TimelineConstants::TOOLBAR_MARGIN));
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

    g_StudioApp.GetCore()->GetDoc()->SetKeyframesManager(m_graphicsScene->keyframeManager());

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

        // make sure the 2 scrollbars stay in sync
        if (m_viewTreeContent->verticalScrollBar()->value() != value) {
            m_viewTimelineContent->verticalScrollBar()->setValue(
                        m_viewTreeContent->verticalScrollBar()->value());
        }
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
        // Keep m_viewTreeContent always positioned at 0
        // This hack is required due to RowTimelineCommentItem (QGraphicsTextItem)
        // ensuring that all views see the text item when it gets focus or content
        // changes with setPlainText(). See QTBUG-71241 and QT3DS-1508.
        if (value != 0)
            m_viewTreeContent->horizontalScrollBar()->setValue(0);
    });

    connect(m_toolbar, &TimelineToolbar::newLayerTriggered, this, [this]() {
        using namespace Q3DStudio;
        CDoc *doc = g_StudioApp.GetCore()->GetDoc();

        // If active instance is component, just bail as we can't add layers to components
        qt3dsdm::Qt3DSDMInstanceHandle rootInstance = doc->GetActiveRootInstance();
        if (m_bridge->GetObjectType(rootInstance) == OBJTYPE_COMPONENT)
            return;

        qt3dsdm::Qt3DSDMSlideHandle slide = doc->GetActiveSlide();
        qt3dsdm::Qt3DSDMInstanceHandle layer = doc->GetActiveLayer();

        SCOPED_DOCUMENT_EDITOR(*doc, QObject::tr("Add Layer"))
            ->CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Layer, layer, slide,
                                       DocumentEditorInsertType::PreviousSibling,
                                       CPt(), PRIMITIVETYPE_UNKNOWN, -1);
    });

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    connect(m_toolbar, &TimelineToolbar::deleteLayerTriggered,
            [=](){ doc->deleteSelectedObject(); });

    connect(m_toolbar, &TimelineToolbar::gotoTimeTriggered, this, [=]() {
        CDoc *doc = g_StudioApp.GetCore()->GetDoc();
        g_StudioApp.GetDialogs()->asyncDisplayTimeEditDialog(doc->GetCurrentViewTime(),
                                                             doc, PLAYHEAD,
                                                             m_graphicsScene->keyframeManager());
    });

    connect(m_toolbar, &TimelineToolbar::firstFrameTriggered, this, []() {
        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(0);
    });

    connect(m_toolbar, &TimelineToolbar::stopTriggered, this, []() {
        g_StudioApp.PlaybackStopNoRestore();
    });

    connect(m_toolbar, &TimelineToolbar::playTriggered, this, [this]() {
        CDoc *doc = g_StudioApp.GetCore()->GetDoc();
        if (getPlaybackMode() == "Stop at end" && doc->isPlayHeadAtEnd())
            g_StudioApp.PlaybackRewind();

        g_StudioApp.PlaybackPlay();
    });

    connect(m_toolbar, &TimelineToolbar::lastFrameTriggered, this, [this]() {
        long dur = m_graphicsScene->ruler()->duration();
        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(dur);
    });

    connect(m_toolbar, &TimelineToolbar::timelineScaleChanged, this, [this](int scale) {
        m_graphicsScene->setTimelineScale(scale);
    });

    connect(m_toolbar, &TimelineToolbar::controllerChanged, this,
            [this](const QString &controller) {
        m_graphicsScene->setControllerText(controller);
    });

    connect(m_toolbar, &TimelineToolbar::variantsFilterToggled, this,
            std::bind(&TimelineWidget::updateVariantsFiltering, this, nullptr, true));

    connect(m_graphicsScene->ruler(), &Ruler::maxDurationChanged, this, [this]() {
        m_graphicsScene->updateTimelineLayoutWidth();
    });

    connect(m_graphicsScene->ruler(), &Ruler::durationChanged, this, [this]() {
        m_graphicsScene->updateControllerLayoutWidth();
    });

    // data model listeners
    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);
    g_StudioApp.GetCore()->GetDispatch()->AddClientPlayChangeListener(this);

    m_asyncUpdateTimer.setInterval(0);
    m_asyncUpdateTimer.setSingleShot(true);
    connect(&m_asyncUpdateTimer, &QTimer::timeout, this, &TimelineWidget::onAsyncUpdate);
}

Q3DStudio::CString TimelineWidget::getPlaybackMode()
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::Qt3DSDMSlideHandle theActiveSlide(doc->GetActiveSlide());
    // clock has passed the end, check whether needs to switch slide
    qt3dsdm::Qt3DSDMInstanceHandle instance = doc->GetStudioSystem()->GetSlideSystem()
                                              ->GetSlideInstance(theActiveSlide);

    CClientDataModelBridge *bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    qt3dsdm::IPropertySystem *propertySystem = doc->GetStudioSystem()->GetPropertySystem();
    qt3dsdm::SValue theValue;
    propertySystem->GetInstancePropertyValue(instance, bridge->GetSlide().m_PlayMode, theValue);
    return qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue)->GetData();
}

TimelineWidget::~TimelineWidget()
{
    CStudioPreferences::SetTimelineSplitterLocation(m_graphicsScene->treeWidth());
    CStudioPreferences::savePreferences();
    m_graphicsScene->keyframeManager()->deselectAllKeyframes();
    delete m_BreadCrumbProvider;
}

QSize TimelineWidget::sizeHint() const
{
    return m_preferredSize;
}

void TimelineWidget::OnNewPresentation()
{
    // Disable scrolling of treeview now that all show related initial singnaling is behind us
    m_viewTreeHeader->disableScrolling();

    // Register callbacks
    auto studioSystem = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    qt3dsdm::IStudioFullSystemSignalProvider *theSignalProvider
            = studioSystem->GetFullSystemSignalProvider();
    m_bridge = studioSystem->GetClientDataModelBridge();

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
    m_connections.push_back(theSignalProvider->ConnectFirstKeyframeDynamicSet(
        std::bind(&TimelineWidget::onFirstKeyframeDynamicSet, this,
                  std::placeholders::_1)));

    // action created/deleted
    m_connections.push_back(theSignalProvider->ConnectActionCreated(
        std::bind(&TimelineWidget::onActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectActionDeleted(
        std::bind(&TimelineWidget::onActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

    // connect property linked/unlinked
    m_connections.push_back(theSignalProvider->ConnectPropertyLinked(
        std::bind(&TimelineWidget::onPropertyLinked, this,
                  std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectPropertyUnlinked(
        std::bind(&TimelineWidget::onPropertyUnlinked, this,
                  std::placeholders::_2, std::placeholders::_3)));

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

    // Reset timeline time
    OnTimeChanged(0);

    // Show timeline widget when presentation is available
    setVisible(true);
}

void TimelineWidget::OnClosingPresentation()
{
    m_connections.clear();
    m_graphicsScene->expandMap().clear();

    // Hide timeline widget when presentation is closed
    setVisible(false);
}

void TimelineWidget::OnTimeChanged(long inTime)
{
    m_graphicsScene->playHead()->setTime(inTime);
    m_toolbar->setTime(inTime);

    double left = m_viewTimelineHeader->horizontalScrollBar()->value()
            + TimelineConstants::PLAYHEAD_W * .5;
    double right = m_viewTimelineHeader->horizontalScrollBar()->value()
            + m_viewTimelineHeader->width() - TimelineConstants::RULER_EDGE_OFFSET;
    double playHeadX = m_graphicsScene->playHead()->x();

    if (playHeadX < left || playHeadX > right) {
        m_viewTimelineContent->ensureVisible(m_graphicsScene->playHead()->x()
                                             - TimelineConstants::PLAYHEAD_W * .5,
                                             m_viewTimelineContent->verticalScrollBar()->value()
                                             + TimelineConstants::ROW_H,
                                             TimelineConstants::PLAYHEAD_W, 0, 0, 0);
    }

    if (inTime <= 0 && g_StudioApp.IsPlaying() && getPlaybackMode() == "Ping"
            && !g_StudioApp.isPlaybackPreviewOn()) {
        g_StudioApp.PlaybackStopNoRestore();
    }
}

void TimelineWidget::onActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                                   const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    Q_UNUSED(inMaster);
    Q_UNUSED(inIndex);

    if (m_activeSlide == inSlide)
        return;

    m_activeSlide = inSlide;

    if (!m_fullReconstruct) {
        m_fullReconstruct = true;
        m_graphicsScene->resetMousePressParams();
        if (!m_asyncUpdateTimer.isActive())
            m_asyncUpdateTimer.start();
    }
}

void TimelineWidget::insertToHandlesMapRecursive(Qt3DSDMTimelineItemBinding *binding)
{
    insertToHandlesMap(binding);

    const QList<ITimelineItemBinding *> children = binding->GetChildren();
    for (auto child : children)
        insertToHandlesMapRecursive(static_cast<Qt3DSDMTimelineItemBinding *>(child));
}

void TimelineWidget::insertToHandlesMap(Qt3DSDMTimelineItemBinding *binding)
{
    m_handlesMap.insert(binding->GetInstance(), binding->getRowTree());
}

void TimelineWidget::onSelectionChange(Q3DStudio::SSelectedValue inNewSelectable)
{
    // Full update will set selection anyway
    if (m_fullReconstruct)
        return;

    qt3dsdm::TInstanceHandleList theInstances = inNewSelectable.GetSelectedInstances();

    // First deselect all items in UI
    m_graphicsScene->rowManager()->clearSelection();

    if (theInstances.size() > 0) {
        for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(theInstances[idx]);

            if (g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->IsInstance(theInstance)) {
                auto *binding = getBindingForHandle(theInstance, m_binding);
                if (binding)
                    m_graphicsScene->rowManager()->setRowSelection(binding->getRowTree(), true);
            }
        }
    }
}

void TimelineWidget::onAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (m_fullReconstruct)
        return;

    if (m_bridge->IsSceneGraphInstance(inInstance)) {
        Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(inInstance, m_binding);

        if (!binding) {
            // if binding is not found, refresh it (so far the only known case where this is needed
            // is when setting a subpresentation on a ref mat row and checking 'Detach material')
            m_fullReconstruct = true;
            if (!m_asyncUpdateTimer.isActive())
                m_asyncUpdateTimer.start();
        } else {
            if (!binding->getRowTree()) { // row doesn't exist
                auto parentInstance = m_bridge->GetParentInstance(inInstance);
                Qt3DSDMTimelineItemBinding *bindingParent = getBindingForHandle(parentInstance,
                                                                                m_binding);
                if (bindingParent) {
                    RowTree *row = m_graphicsScene->rowManager()
                                   ->createRowFromBinding(binding, bindingParent->getRowTree());
                    row->updateSubpresentations();
                    insertToHandlesMap(binding);

                    // refresh the created object variants if it has a variants property set.
                    if (m_bridge->GetObjectType(inInstance) & OBJTYPE_IS_VARIANT) {
                        const auto propertySystem = g_StudioApp.GetCore()->GetDoc()
                                                    ->GetPropertySystem();
                        qt3dsdm::SValue sValue;
                        if (propertySystem->GetInstancePropertyValue(inInstance,
                                                        m_bridge->getVariantsProperty(inInstance),
                                                        sValue)) {
                            if (qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->GetLength() != 0)
                                refreshVariants(inInstance);
                        }
                    }
                } else {
                    qWarning() << "Binding parent was not found.";
                }
            }
        }
    }
}

void TimelineWidget::onAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (m_fullReconstruct)
        return;

    RowTree *row = m_handlesMap.value(inInstance);
    if (row) { // scene object exists
        row->updateSubpresentations(-1);
        m_graphicsScene->rowManager()->deleteRow(row);
        m_handlesMap.remove(inInstance);
        m_graphicsScene->expandMap().remove(inInstance);
        // Ensure row deletions are finalized
        if (!m_asyncUpdateTimer.isActive())
            m_asyncUpdateTimer.start();
    }
}

void TimelineWidget::onAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    if (m_fullReconstruct)
        return;

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
                                 propRow->rowTimeline(), kf->GetTime(), false).at(0);

                kf->setUI(kfUI);
                kfUI->binding = static_cast<Qt3DSDMTimelineKeyframe *>(kf);
                kfUI->dynamic = kf->IsDynamic();
            }

            propRow->update();
        }
    }
}

void TimelineWidget::onAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    if (m_fullReconstruct)
        return;

    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(parentInstance, m_binding);
    if (binding) {
        ITimelineItemProperty *propBinding = binding->GetPropertyBinding(property);
        // this is needed because onAnimationDeleted can be triggered for unlinked property on
        // different slide in undo/redo situations
        bool propAnimated = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()
                            ->GetAnimationSystem()->IsPropertyAnimated(parentInstance, property);

        if (propBinding && !propAnimated) {
            m_graphicsScene->rowManager()->deleteRow(propBinding->getRowTree());
            binding->RemovePropertyRow(property);
            m_keyframeChangesMap.insert(parentInstance, property);
            // Ensure row deletions are finalized
            if (!m_asyncUpdateTimer.isActive())
                m_asyncUpdateTimer.start();
        }
    }
}

void TimelineWidget::onKeyframeInserted(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                                        qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe)
{
    if (m_fullReconstruct)
        return;

    refreshKeyframe(inAnimation, inKeyframe, ETimelineKeyframeTransaction_Add);
}

void TimelineWidget::onKeyframeDeleted(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                                       qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe)
{
    if (m_fullReconstruct)
        return;

    refreshKeyframe(inAnimation, inKeyframe, ETimelineKeyframeTransaction_Delete);
}

void TimelineWidget::onKeyframeUpdated(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe)
{
    if (m_fullReconstruct)
        return;

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
        if (binding) {
            binding->RefreshPropertyKeyframe(theAnimationInfo.m_Property, inKeyframe,
                                             inTransaction);

            // update UI asynchronously to make sure binding is completely up to date.
            m_keyframeChangesMap.insert(theAnimationInfo.m_Instance, theAnimationInfo.m_Property);
            if (!m_asyncUpdateTimer.isActive())
                m_asyncUpdateTimer.start();
        }
    }
}

void TimelineWidget::onFirstKeyframeDynamicSet(qt3dsdm::Qt3DSDMAnimationHandle inAnimation)
{
    refreshKeyframe(inAnimation, 0, ETimelineKeyframeTransaction_DynamicChanged);
}

void TimelineWidget::updateActionStates(const QSet<RowTree *> &rows)
{
    for (RowTree *row : rows) {
        Qt3DSDMTimelineItemBinding *binding =
                static_cast<Qt3DSDMTimelineItemBinding *>(row->getBinding());

        RowTree::ActionStates states = RowTree::ActionState::None;
        if (binding->HasAction(true)) // has master action
            states |= RowTree::ActionState::MasterAction;
        else if (binding->HasAction(false)) // has action
            states |= RowTree::ActionState::Action;

        if (binding->ChildrenHasAction(true)) // children have master action
            states |= RowTree::ActionState::MasterChildAction;
        else if (binding->ChildrenHasAction(false)) // children have action
            states |= RowTree::ActionState::ChildAction;

        if (row->isComponent()) {
            if (binding->ComponentHasAction(true)) // component has master action
                states |= RowTree::ActionState::MasterComponentAction;
            else if (binding->ComponentHasAction(false)) // component has action
                states |= RowTree::ActionState::ComponentAction;
        }
        row->setActionStates(states);
    }
}

void TimelineWidget::setTreeWidth(int width)
{
    int treeWidth = qBound(int(TimelineConstants::TREE_MIN_W), width,
                           int(TimelineConstants::TREE_MAX_W));

    m_viewTreeHeader->setFixedWidth(treeWidth);
    m_viewTreeContent->setFixedWidth(treeWidth);
    m_graphicsScene->updateTreeWidth(treeWidth);
}

void TimelineWidget::onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                       qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    if (m_fullReconstruct)
        return;

    if (!m_bridge->IsSceneGraphInstance(inInstance))
        return;

    const SDataModelSceneAsset &asset = m_bridge->GetSceneAsset();
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    auto ctrldPropHandle = doc->GetPropertySystem()
            ->GetAggregateInstancePropertyByName(inInstance, L"controlledproperty");

    if (inProperty == asset.m_Eyeball || inProperty == asset.m_Locked || inProperty == asset.m_Shy
            || inProperty == asset.m_StartTime || inProperty == asset.m_EndTime
            || inProperty == m_bridge->GetNameProperty() || inProperty == ctrldPropHandle) {
        m_dirtyProperties.insert(inInstance, inProperty);
        if (!m_asyncUpdateTimer.isActive())
            m_asyncUpdateTimer.start();
    } else if (inProperty == m_bridge->GetSceneImage().m_SubPresentation
               || (inProperty == m_bridge->GetSourcePathProperty()
                   && m_bridge->GetObjectType(inInstance) == OBJTYPE_LAYER)) {
        m_subpresentationChanges.insert(inInstance);
        if (!m_asyncUpdateTimer.isActive())
            m_asyncUpdateTimer.start();
    } else if (inProperty == m_bridge->getVariantsProperty(inInstance)) {
        qt3dsdm::SValue sValue;
        if (doc->GetPropertySystem()->GetInstancePropertyValue(inInstance, inProperty, sValue)) {
            QString propVal = qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->toQString();
            if (!propVal.isEmpty()) {
                QStringList tagPairs = propVal.split(QLatin1Char(','));
                QStringList groups;
                for (int i = 0; i < tagPairs.size(); ++i) {
                    QString group = tagPairs[i].left(tagPairs[i].indexOf(QLatin1Char(':')));
                    if (!groups.contains(group))
                        groups.append(group);
                }

                m_variantsMap[inInstance] = groups;
            } else {
                m_variantsMap[inInstance].clear();
            }

            if (!m_asyncUpdateTimer.isActive())
                m_asyncUpdateTimer.start();
        }
    }
}

void TimelineWidget::onAsyncUpdate()
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();

    if (m_fullReconstruct) {
        m_translationManager->Clear();
        m_binding = static_cast<Qt3DSDMTimelineItemBinding *>(
                    m_translationManager->GetOrCreate(
                        doc->GetStudioSystem()->GetSlideSystem()->GetSlideInstance(m_activeSlide)));
        m_graphicsScene->rowManager()->recreateRowsFromBinding(m_binding);
        m_handlesMap.clear();
        insertToHandlesMapRecursive(m_binding);
        updateActionStates(m_handlesMap.values().toSet());
        m_navigationBar->updateNavigationItems(m_BreadCrumbProvider);
        m_graphicsScene->updateSnapSteps();
        m_fullReconstruct = false;
        m_graphicsScene->updateController();
        onSelectionChange(doc->GetSelectedValue());
        m_toolbar->setNewLayerEnabled(!m_graphicsScene->rowManager()->isComponentRoot());
        refreshVariants();
        updateVariantsFiltering();

        // update sub-presentation indicators
        for (auto *row : qAsConst(m_handlesMap))
            row->updateSubpresentations();
    } else {
        if (!m_moveMap.isEmpty()) {
            // Flip the hash around so that we collect moves by parent.
            // We can't do this with m_moveMap originally, as things break if
            // same row receives consecutive moves to different parents.
            QMultiHash<int, int> flippedMap;
            QHashIterator<int, int> it(m_moveMap);
            while (it.hasNext()) {
                it.next();
                flippedMap.insert(it.value(), it.key());
            }
            const auto parentHandles = flippedMap.keys();
            QSet<RowTree *> expandRows;
            for (const auto parentHandle : parentHandles) {
                QSet<int> movedInstances(flippedMap.values(parentHandle).toSet());
                RowTree *rowParent = m_handlesMap.value(parentHandle);
                if (rowParent) {
                    Qt3DSDMTimelineItemBinding *bindingParent
                            = static_cast<Qt3DSDMTimelineItemBinding *>(rowParent->getBinding());
                    if (bindingParent) {
                        // Resolve indexes for handles. QMap used for its automatic sorting by keys.
                        QMap<int, int> indexMap;
                        bindingParent->getTimeContextIndices(movedInstances, indexMap);
                        QMapIterator<int, int> indexIt(indexMap);
                        while (indexIt.hasNext()) {
                            indexIt.next();
                            RowTree *row = m_handlesMap.value(indexIt.value());
                            if (row) {
                                bool isReparent = rowParent != row->parentRow();
                                if (isReparent)
                                    row->updateSubpresentations(-1);
                                rowParent->addChildAt(row, indexIt.key());
                                if (isReparent)
                                    row->updateSubpresentations(1);
                            }
                        }
                        expandRows.insert(rowParent);
                    }
                }
            }

            // Make sure selections on UI matches bindings
            onSelectionChange(doc->GetSelectedValue());

            // Expand the parents of the added rows, but only for topmost ancestors of the moved
            // rows as expanding all moved rows indiscriminately would not work intuitively
            // in case of e.g. mass delete undo.
            // Rest of expandRows will be force-updated to their current state to ensure
            // their children are in proper state. This is relevant in cases like grouping,
            // where existing potentially visible rows are moved under newly created group,
            // which is collapsed by default.
            for (RowTree *row : qAsConst(expandRows)) {
                if (!expandRows.contains(row->parentRow()))
                    m_graphicsScene->rowManager()->ensureRowExpandedAndVisible(row, true);
                else
                    row->updateExpandStatus(row->expandState(), false, true);
            }
        }
        // Update properties
        if (!m_dirtyProperties.isEmpty()) {
            const SDataModelSceneAsset &asset = m_bridge->GetSceneAsset();
            qt3dsdm::Qt3DSDMPropertyHandle nameProp = m_bridge->GetNameProperty();
            const auto instances = m_dirtyProperties.keys();
            QSet<RowTree *> updateArrowParents;
            for (int instance : instances) {
                bool filterProperty = false;
                bool timeProperty = false;
                bool nameProperty = false;
                const auto props = m_dirtyProperties.values(instance);
                const auto ctrldPropHandle =
                        doc->GetPropertySystem()->GetAggregateInstancePropertyByName(
                            instance, L"controlledproperty");
                for (auto prop : props) {
                    filterProperty = filterProperty || prop == asset.m_Eyeball
                            || prop == asset.m_Locked || prop == asset.m_Shy
                            || prop == ctrldPropHandle;
                    timeProperty = timeProperty
                            || prop == asset.m_StartTime || prop == asset.m_EndTime;
                    nameProperty = nameProperty || prop == nameProp;
                }
                if (filterProperty || timeProperty || nameProperty) {
                    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(instance, m_binding);
                    if (binding) {
                        RowTree *row = binding->getRowTree();
                        if (row) {
                            if (timeProperty) {
                                row->rowTimeline()->updateDurationFromBinding();
                                m_graphicsScene->rowManager()->updateRulerDuration();
                            }
                            if (filterProperty) {
                                row->updateFromBinding();
                                m_graphicsScene->rowManager()->updateFiltering(row);
                                // Filtering changes to children affect arrow visibility in parents.
                                if (row->parentRow())
                                    updateArrowParents.insert(row->parentRow());

                                update();
                            }
                            if (nameProperty)
                                row->updateLabel();
                        }
                    }
                }
            }
            for (RowTree *row : qAsConst(updateArrowParents))
                row->updateArrowVisibility();
            m_graphicsScene->updateSnapSteps();
        }
        if (!m_actionChanges.isEmpty()) {
            QSet<RowTree *> rowSet;
            for (int id : qAsConst(m_actionChanges)) {
                RowTree *row = m_handlesMap.value(id);
                if (row) {
                    rowSet.insert(row);
                    RowTree *parentRow = row->parentRow();
                    while (parentRow) {
                        rowSet.insert(parentRow);
                        parentRow = parentRow->parentRow();
                    }
                }
            }
            updateActionStates(rowSet);
        }

        if (!m_subpresentationChanges.isEmpty()) {
            for (int id : qAsConst(m_subpresentationChanges)) {
                RowTree *row = m_handlesMap.value(id);
                if (row)
                    row->updateSubpresentations();
            }
        }

        if (!m_keyframeChangesMap.isEmpty()) {
            const auto objects = m_keyframeChangesMap.keys();
            for (int object : objects) {
                RowTree *row = m_handlesMap.value(object);
                if (row) {
                    const auto properties = m_keyframeChangesMap.values(object);
                    row->rowTimeline()->updateKeyframesFromBinding(properties);
                }
            }
            m_graphicsScene->updateSnapSteps();
        }

        if (!m_variantsMap.isEmpty()) {
            const auto instances = m_variantsMap.keys();
            for (int instance : instances) {
                if (m_handlesMap.contains(instance)) {
                    RowTree *row = m_handlesMap[instance];
                    if (row) {
                        row->updateVariants(m_variantsMap[instance]); // variants groups names
                        updateVariantsFiltering(row);
                    }
                }
            }
        }
    }
    m_dirtyProperties.clear();
    m_moveMap.clear();
    m_actionChanges.clear();
    m_variantsMap.clear();
    m_subpresentationChanges.clear();
    m_keyframeChangesMap.clear();
    m_graphicsScene->rowManager()->finalizeRowDeletions();
}

void TimelineWidget::onActionEvent(qt3dsdm::Qt3DSDMActionHandle inAction,
                                   qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                   qt3dsdm::Qt3DSDMInstanceHandle inOwner)
{
    Q_UNUSED(inAction)
    Q_UNUSED(inSlide)

    if (m_fullReconstruct)
        return;

    m_actionChanges.insert(inOwner);
    if (!m_asyncUpdateTimer.isActive())
        m_asyncUpdateTimer.start();
}

void TimelineWidget::onPropertyLinked(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                      qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    if (m_fullReconstruct)
        return;

    Qt3DSDMTimelineItemBinding *binding = getBindingForHandle(inInstance, m_binding);

    if (binding) {
        ITimelineItemProperty *propBinding = binding->GetPropertyBinding(inProperty);

        if (propBinding) {
            RowTree *propRow = binding->GetPropertyBinding(inProperty)->getRowTree();

            // this call deletes and recreates the property binding so we need to reconnect the
            // property binding and its RowTree, and the keyframes also
            binding->OnPropertyLinked(inProperty);

            propBinding = binding->GetPropertyBinding(inProperty);
            propBinding->setRowTree(propRow);
            propRow->setPropBinding(propBinding);

            // recreate and connect prop row keyframes
            m_graphicsScene->keyframeManager()->deleteKeyframes(propRow->rowTimeline(), false);
            for (int i = 0; i < propBinding->GetKeyframeCount(); i++) {
                IKeyframe *kf = propBinding->GetKeyframeByIndex(i);
                Keyframe *kfUI = m_graphicsScene->keyframeManager()->insertKeyframe(
                            propRow->rowTimeline(), kf->GetTime(), false).at(0);

                kf->setUI(kfUI);
                kfUI->binding = static_cast<Qt3DSDMTimelineKeyframe *>(kf);
                kfUI->dynamic = kf->IsDynamic();
            }
        }
    }
}

void TimelineWidget::onPropertyUnlinked(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                        qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    onPropertyLinked(inInstance, inProperty);
}

void TimelineWidget::onChildAdded(int inParent, int inChild, long inIndex)
{
    Q_UNUSED(inIndex)

    if (m_fullReconstruct)
        return;

    // Handle row moves async, as we won't be able to get the final order correct otherwise
    m_moveMap.insert(inChild, inParent);
    m_actionChanges.insert(inParent);
    if (!m_asyncUpdateTimer.isActive())
        m_asyncUpdateTimer.start();
}

void TimelineWidget::onChildRemoved(int inParent, int inChild, long inIndex)
{
    Q_UNUSED(inParent)
    Q_UNUSED(inChild)
    Q_UNUSED(inIndex)

    m_actionChanges.insert(inParent);
    if (!m_asyncUpdateTimer.isActive())
        m_asyncUpdateTimer.start();

    // Note: Actual child removal handling unimplemented by design, see QT3DS-1684
}

void TimelineWidget::onChildMoved(int inParent, int inChild, long inOldIndex,
                                  long inNewIndex)
{
    Q_UNUSED(inOldIndex)

    // Move and add are essentially the same operation
    onChildAdded(inParent, inChild, inNewIndex);
}

CDropTarget *TimelineWidget::FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags,
                                               EStudioObjectType objectType,
                                               Q3DStudio::DocumentEditorFileType::Enum fileType)
{
    Q_UNUSED(inFlags)

    CTimeLineDropTarget *theTarget = new CTimeLineDropTarget();

    int mouseY = inMousePoint.y - m_navigationBar->height()
                 + viewTreeContent()->verticalScrollBar()->value()
                 - viewTreeContent()->verticalScrollBar()->minimum();
    RowMover *mover = m_graphicsScene->rowMover();
    mover->updateTargetRow(QPointF(inMousePoint.x, mouseY), objectType, fileType);

    if (mover->insertionTarget() && !mover->insertionTarget()->isProperty()) {
        mover->insertionTarget()->getBinding()->SetDropTarget(theTarget);

        switch (mover->insertionType()) {
        case Q3DStudio::DocumentEditorInsertType::LastChild:
            theTarget->SetDestination(EDROPDESTINATION_ON);
            break;
        case Q3DStudio::DocumentEditorInsertType::PreviousSibling:
            theTarget->SetDestination(EDROPDESTINATION_ABOVE);
            break;
        default:
            theTarget->SetDestination(EDROPDESTINATION_BELOW);
            break;
        }
    }
    m_graphicsScene->updateAutoScrolling(mouseY);

    return theTarget;
}

void TimelineWidget::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inFlags)

    if (inPoint.x == -1 && inPoint.y == -1) { // drag leave
        // upon cancelling a DnD, the mouse press event fires, this bool is to prevent that
        m_blockMousePress = true;
        QTimer::singleShot(500, [this]() {
            m_blockMousePress = false;
        });
    }
}

bool TimelineWidget::blockMousePress() const
{
    return m_blockMousePress;
}

CPt TimelineWidget::GetPreferredSize()
{
    return CPt(m_preferredSize.width(), m_preferredSize.height());
}

void TimelineWidget::SetSize(long inX, long inY)
{
    setFixedSize(inX, inY);
}

// If views are interactive they block the DnD. If we could think of a way to make them do not block
// DnD, then this method can be removed (and it's callers)
void TimelineWidget::enableDnD(bool b)
{
    m_viewTreeHeader->setEnabled(!b);
    m_viewTreeContent->setEnabled(!b);
    m_viewTimelineHeader->setEnabled(!b);
    m_viewTimelineContent->setEnabled(!b);

    if (!b) { // object successfully dropped on the timeline tree
        m_graphicsScene->rowMover()->end(true);
        m_graphicsScene->stopAutoScroll();
    }
}

Qt3DSDMTimelineItemBinding *TimelineWidget::getBindingForHandle(int handle,
                                                Qt3DSDMTimelineItemBinding *binding) const
{
    const RowTree *row = m_handlesMap.value(handle);
    if (row && row->getBinding())
        return static_cast<Qt3DSDMTimelineItemBinding *>(row->getBinding());

    if (binding) {
        if (binding->GetInstance().GetHandleValue() == handle)
            return binding;

        const QList<ITimelineItemBinding *> children = binding->GetChildren();
        for (auto child : children) {
            Qt3DSDMTimelineItemBinding *b = getBindingForHandle(handle,
                                            static_cast<Qt3DSDMTimelineItemBinding *>(child));

            if (b)
                return b;
        }
    }
    return nullptr;
}

void TimelineWidget::mousePressEvent(QMouseEvent *event)
{
    if (childAt(event->pos()) == m_splitter)
        m_splitterPressed = true;
    g_StudioApp.setLastActiveView(this);
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_splitterPressed)
        setTreeWidth(event->pos().x() - m_splitter->size().width() * .5);
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
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

bool TimelineWidget::dndActive() const
{
    return m_graphicsScene->rowMover()->isActive();
}

bool TimelineWidget::hasSelectedKeyframes() const
{
    return m_graphicsScene->keyframeManager()->hasSelectedKeyframes();
}

QVector<RowTree *> TimelineWidget::selectedRows() const
{
    return m_graphicsScene->rowManager()->selectedRows();
}

void TimelineWidget::openBarColorDialog()
{
    auto rows = selectedRows();
    if (rows.isEmpty())
        return;

    // Note: Setup color dialog with bar color of last selected row as it can only default to one.
    QColor previousColor = rows.first()->rowTimeline()->barColor();
    CDialogs *dialogs = g_StudioApp.GetDialogs();
    connect(dialogs, &CDialogs::onColorChanged, this, &TimelineWidget::onTimeBarColorChanged);
    QColor selectedColor = dialogs->displayColorDialog(previousColor);
    disconnect(dialogs, &CDialogs::onColorChanged, this, &TimelineWidget::onTimeBarColorChanged);
    setSelectedTimeBarsColor(selectedColor, selectedColor == previousColor);
}

void TimelineWidget::onTimeBarColorChanged(const QColor &color)
{
    setSelectedTimeBarsColor(color, true);
}

// Set the color of all currently selected timeline bars.
// When preview, only set the UI without property changes.
void TimelineWidget::setSelectedTimeBarsColor(const QColor &color, bool preview)
{
    using namespace Q3DStudio; // Needed for SCOPED_DOCUMENT_EDITOR macro
    const auto rows = selectedRows();
    for (RowTree *row : rows) {
        row->rowTimeline()->setBarColor(color);
        if (!preview) {
            Qt3DSDMTimelineItemBinding *timelineItemBinding =
                    static_cast<Qt3DSDMTimelineItemBinding *>(row->getBinding());
            SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(),
                                   QObject::tr("Set Timebar Color"))
                ->SetTimebarColor(timelineItemBinding->GetInstanceHandle(), color);
        }
    }
}

void TimelineWidget::refreshVariants(int instance)
{
    const auto propertySystem = g_StudioApp.GetCore()->GetDoc()->GetPropertySystem();
    QVector<int> instances;
    if (instance)
        instances << instance;
    else
        instances = g_StudioApp.GetCore()->GetDoc()->getVariantInstances();

    for (auto instance : qAsConst(instances)) {
        if (!m_handlesMap.contains(instance))
            continue;

        qt3dsdm::SValue sValue;
        if (propertySystem->GetInstancePropertyValue(instance,
                                                     m_bridge->getVariantsProperty(instance),
                                                     sValue)) {
            QString propVal = qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->toQString();
            if (!propVal.isEmpty()) {
                QStringList tagPairs = propVal.split(QLatin1Char(','));
                QStringList groups;
                for (int i = 0; i < tagPairs.size(); ++i) {
                    QString group = tagPairs[i].left(tagPairs[i].indexOf(QLatin1Char(':')));
                    if (!groups.contains(group))
                        groups.append(group);
                }

                m_handlesMap[instance]->updateVariants(groups);
            } else {
                m_handlesMap[instance]->updateVariants({});
            }
        }
    }
}

void TimelineWidget::updateVariantsFiltering(RowTree *row, bool force)
{
    if (force || m_toolbar->isVariantsFilterOn())
        m_graphicsScene->rowManager()->updateFiltering(row);
}
