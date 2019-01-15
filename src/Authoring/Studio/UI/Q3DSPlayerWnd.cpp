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

#include "Q3DSPlayerWnd.h"
#include "IDragable.h"
#include "StudioApp.h"
#include "IStudioRenderer.h"
#include "Core.h"
#include "HotKeys.h"
#include "Dispatch.h"
#include "SceneDropTarget.h"
#include "Q3DStudioRenderer.h"
#include "SceneView.h"
#include "StudioPreferences.h"
#include "StudioProjectSettings.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMSlides.h"
#include "FileDropSource.h"
#include "Dialogs.h"
#include "StudioUtils.h"

#include <QtGui/qoffscreensurface.h>
#include <QtGui/qopenglcontext.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qscrollbar.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>
#include <QtCore/qtimer.h>

namespace Q3DStudio
{

template<typename T>
T even(const T val)
{
    // handle negative values
    T corr = (val > 0) ? -1 : 1;
    return (val % 2) ? (val + corr) : val;
}

static bool compareSurfaceFormatVersion(const QSurfaceFormat &a, const QSurfaceFormat &b)
{
    if (a.renderableType() != b.renderableType())
        return false;
    if (a.majorVersion() != b.majorVersion())
        return false;
    if (a.minorVersion() > b.minorVersion())
        return false;
    return true;
}

static QSurfaceFormat selectSurfaceFormat(QOpenGLWidget *window)
{
    struct ContextVersion {
        int major;
        int minor;
        bool gles;
    };

    ContextVersion versions[] = {
        {4, 5, false},
        {4, 4, false},
        {4, 3, false},
        {4, 2, false},
        {4, 1, false},
        {3, 3, false},
        {2, 1, false},
        {3, 2, true},
        {3, 1, true},
        {3, 0, true},
        {2, 1, true},
        {2, 0, true},
    };

    QSurfaceFormat result = window->format();
    bool valid = false;

    for (const auto &ver : versions) {
        // make an offscreen surface + context to query version
        QScopedPointer<QOffscreenSurface> offscreenSurface(new QOffscreenSurface);

        QSurfaceFormat format = window->format();
        if (ver.gles) {
            format.setRenderableType(QSurfaceFormat::OpenGLES);
        } else {
            format.setRenderableType(QSurfaceFormat::OpenGL);
            if (ver.major >= 2)
                format.setProfile(QSurfaceFormat::CoreProfile);
        }
        format.setMajorVersion(ver.major);
        format.setMinorVersion(ver.minor);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);

        offscreenSurface->setFormat(format);
        offscreenSurface->create();
        Q_ASSERT(offscreenSurface->isValid());

        QScopedPointer<QOpenGLContext> queryContext(new QOpenGLContext);
        queryContext->setFormat(format);
        if (queryContext->create() && compareSurfaceFormatVersion(format, queryContext->format())) {
            valid = true;
            result = format;
            break;
        }
    } // of version test iteration

    if (!valid)
        qFatal("Unable to select suitable OpenGL context");

    qDebug() << Q_FUNC_INFO << "selected surface format:" << result;
    QSurfaceFormat::setDefaultFormat(result);
    return result;
}

Q3DSPlayerWnd::Q3DSPlayerWnd(QWidget *parent)
    : QScrollArea(parent)
    , m_mouseDown(false)
    , m_renderWindow(new RenderWindow())
    , m_ViewMode(VIEW_SCENE)
{
    m_widget = QWidget::createWindowContainer(m_renderWindow, this);
    m_widget->setMinimumSize(800, 600);
    m_widget->setMaximumSize(1920, 1080);
    m_widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_renderWindow->setFlags(Qt::WindowTransparentForInput);
    m_renderWindow->m_container = m_widget;
    setWidget(m_widget);

    setAcceptDrops(true);
    RegisterForDnd(this);
    AddMainFlavor(QT3DS_FLAVOR_FILE);
    AddMainFlavor(QT3DS_FLAVOR_ASSET_UICFILE);
    AddMainFlavor(QT3DS_FLAVOR_ASSET_LIB);
    AddMainFlavor(QT3DS_FLAVOR_BASIC_OBJECTS);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_previousToolMode = g_StudioApp.GetToolMode();

    Q3DStudio::IStudioRenderer &theRenderer(g_StudioApp.getRenderer());
    if (!theRenderer.IsInitialized()) {
        try {
            theRenderer.Initialize(m_renderWindow);

            // Connect using signal name because IStudioRenderer interface doesn't have the signal
            QObject *obj = dynamic_cast<QObject *>(&theRenderer);
            if (obj) {
                QObject::connect(obj, SIGNAL(objectPicked(int)),
                                 this, SLOT(handleObjectPicked(int)));
            }
        } catch (...) {
            QMessageBox::critical(this, tr("Fatal Error"),
                                  tr("Unable to initialize OpenGL.\nThis may be because your "
                                     "graphic device is not sufficient, or simply because your "
                                     "driver is too old.\n\nPlease try upgrading your graphics "
                                     "driver and try again."));
            exit(1);
        }
    }
}

Q3DSPlayerWnd::~Q3DSPlayerWnd()
{

}

void Q3DSPlayerWnd::resizeEvent(QResizeEvent *event)
{
    setScrollRanges();
}

void Q3DSPlayerWnd::mouseMoveEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        QMouseEvent e = *event;
        e.setLocalPos(sr.scenePoint(e.pos()));
        sr.engine()->handleMouseMoveEvent(&e);
    }

    if (m_mouseDown) {
        long theModifierKeys = 0;
        if (event->buttons() & Qt::LeftButton
                || (!g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance().Valid()
                && !isDeploymentView())) {
            // When in edit camera view and nothing is selected, all buttons are mapped
            // as left button. That is how camera control tools work, they are all
            // assuming left button.
            theModifierKeys = CHotKeys::MOUSE_LBUTTON | CHotKeys::GetCurrentKeyModifiers();
        } else if (event->buttons() & Qt::RightButton) {
            theModifierKeys = CHotKeys::MOUSE_RBUTTON | CHotKeys::GetCurrentKeyModifiers();
        } else if (event->buttons() & Qt::MiddleButton) {
            theModifierKeys = CHotKeys::MOUSE_MBUTTON | CHotKeys::GetCurrentKeyModifiers();
        }
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDrag(
                    SceneDragSenderType::Matte, event->pos(), g_StudioApp.GetToolMode(),
                    theModifierKeys);
    } else {
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseMove(
                    SceneDragSenderType::SceneWindow, event->pos());
    }
}

void Q3DSPlayerWnd::mousePressEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        QMouseEvent e = *event;
        e.setLocalPos(sr.scenePoint(e.pos()));
        sr.engine()->handleMousePressEvent(&e);
    }

    g_StudioApp.setLastActiveView(this);

    long toolMode = g_StudioApp.GetToolMode();
    const Qt::MouseButton btn = event->button();
    bool toolChanged = false;

    if (!isDeploymentView() && (event->modifiers() & Qt::AltModifier)) {
        // We are in edit camera view, so we are in Alt-click camera tool
        // controlling mode
        m_mouseDown = true;
        if (btn == Qt::MiddleButton) {
            // Alt + Wheel Click
            toolMode = STUDIO_TOOLMODE_CAMERA_PAN;
            toolChanged = true;
        } else if (btn == Qt::LeftButton) {
            // Alt + Left Click
            if (g_StudioApp.getRenderer().DoesEditCameraSupportRotation(
                        g_StudioApp.getRenderer().GetEditCamera())) {
                toolMode = STUDIO_TOOLMODE_CAMERA_ROTATE;
                toolChanged = true;
            }
        } else if (btn == Qt::RightButton) {
            // Alt + Right Click
            toolMode = STUDIO_TOOLMODE_CAMERA_ZOOM;
            toolChanged = true;
        }

        if (toolChanged) {
            g_StudioApp.SetToolMode(toolMode);
            Q_EMIT Q3DSPlayerWnd::toolChanged();
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(SceneDragSenderType::Matte,
                                                                     event->pos(), toolMode);
        }
    } else {
        if (btn == Qt::LeftButton || btn == Qt::RightButton) {
            // Pause playback for the duration of the mouse click
            if (g_StudioApp.IsPlaying()) {
                g_StudioApp.PlaybackStopNoRestore();
                m_resumePlayOnMouseRelease = true;
            } else {
                m_resumePlayOnMouseRelease = false;
            }

            toolMode = g_StudioApp.GetToolMode();
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(
                        SceneDragSenderType::SceneWindow, event->pos(), toolMode);
            m_mouseDown = true;
        } else if (btn == Qt::MiddleButton) {
            event->ignore();
        }
    }
}

void Q3DSPlayerWnd::mouseReleaseEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        QMouseEvent e = *event;
        e.setLocalPos(sr.scenePoint(e.pos()));
        sr.engine()->handleMouseReleaseEvent(event);
    }

    const Qt::MouseButton btn = event->button();

    if (!isDeploymentView()) {
        // We are in edit camera view
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(SceneDragSenderType::Matte);
        g_StudioApp.GetCore()->CommitCurrentCommand();
        m_mouseDown = false;
        // Restore normal tool mode
        g_StudioApp.SetToolMode(m_previousToolMode);
        Q_EMIT toolChanged();
    } else {
        if (btn == Qt::LeftButton || btn == Qt::RightButton) {
            g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(
                        SceneDragSenderType::SceneWindow);
            g_StudioApp.GetCore()->CommitCurrentCommand();
            m_mouseDown = false;
            if (m_resumePlayOnMouseRelease) {
                m_resumePlayOnMouseRelease = false;
                g_StudioApp.PlaybackPlay();
            }
        } else if (btn == Qt::MiddleButton) {
            event->ignore();
        }
    }
}

void Q3DSPlayerWnd::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (CStudioApp::hasProfileUI()) {
        Q3DStudioRenderer &sr(static_cast<Q3DStudioRenderer &>(g_StudioApp.getRenderer()));
        QMouseEvent e = *event;
        e.setLocalPos(sr.scenePoint(e.pos()));
        sr.engine()->handleMouseDoubleClickEvent(&e);
    }

    g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDblClick(
                SceneDragSenderType::SceneWindow, event->pos());
}

bool Q3DSPlayerWnd::OnDragWithin(CDropSource &inSource)
{
    CSceneViewDropTarget theTarget;
    const bool currentAccept = theTarget.Accept(inSource);
    bool accept = currentAccept;

    if (theTarget.sourceObjectType() == OBJTYPE_MATERIALDATA) {
        // Always use previous pending request accept value, since this type of acceptance
        // cannot be determined synchronously
        accept = m_objectRequestData.m_instance != 0;
        inSource.SetHasValidTarget(accept);
        m_objectRequestData.m_dropping = false;
    } else {
        m_objectRequestData.m_instance = 0;
    }

    return accept;
}

bool Q3DSPlayerWnd::OnDragReceive(CDropSource &inSource)
{
    CSceneViewDropTarget theTarget;
    Q_EMIT dropReceived();

    theTarget.Drop(inSource);

    if (theTarget.sourceObjectType() == OBJTYPE_MATERIALDATA) {
        // Material data is always a file source, so this cast should be safe
        auto fileSource = static_cast<CFileDropSource *>(&inSource);
        m_objectRequestData.m_matFilePath = fileSource->filePath();
        m_objectRequestData.m_dropping = true;
    }

    return true; // theTarget.Drop() always returns true and the return value isn't used anyway
}

void Q3DSPlayerWnd::OnDragLeave()
{
    m_objectRequestData.clear();
}

void Q3DSPlayerWnd::handleObjectPicked(int instance)
{
    const auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()
            ->GetClientDataModelBridge();
    m_objectRequestData.m_instance = 0;
    if (instance && bridge->GetObjectType(instance) == OBJTYPE_MODEL)
        m_objectRequestData.m_instance = instance;

    if (m_objectRequestData.m_dropping && m_objectRequestData.m_instance) {
        CDoc *doc = g_StudioApp.GetCore()->GetDoc();
        const auto editor = doc->getSceneEditor();
        std::vector<qt3dsdm::Qt3DSDMInstanceHandle> children;
        editor->GetChildren(editor->GetAssociatedSlide(m_objectRequestData.m_instance),
                            m_objectRequestData.m_instance, children);
        qt3dsdm::Qt3DSDMInstanceHandle matInstance;
        for (auto &child : children) {
            const auto childType = bridge->GetObjectType(child);
            if (childType == OBJTYPE_REFERENCEDMATERIAL || childType == OBJTYPE_MATERIAL
                    || childType == OBJTYPE_CUSTOMMATERIAL) {
                matInstance = child;
                break;
            }
        }
        if (matInstance.Valid()) {
            // Logic copied from CFileDropSource::GenerateAssetCommand
            if (!QFileInfo(m_objectRequestData.m_matFilePath).completeBaseName()
                    .contains(QLatin1Char('#'))) {
                const auto doc = g_StudioApp.GetCore()->GetDoc();
                { // Scope for the ScopedDocumentEditor
                    Q3DStudio::ScopedDocumentEditor sceneEditor(
                                Q3DStudio::SCOPED_DOCUMENT_EDITOR(*doc, QString()));
                    QString name;
                    QMap<QString, QString> values;
                    QMap<QString, QMap<QString, QString>> textureValues;
                    sceneEditor->getMaterialInfo(m_objectRequestData.m_matFilePath, name, values,
                                                 textureValues);
                    const auto material = sceneEditor->getOrCreateMaterial(
                                m_objectRequestData.m_matFilePath);
                    sceneEditor->setMaterialValues(material, values, textureValues);
                }
                // Several aspects of the editor are not updated correctly
                // if the data core is changed without a transaction
                // The above scope completes the transaction for creating a new material
                // Next the added undo has to be popped from the stack
                // TODO: Find a way to update the editor fully without a transaction
                doc->GetCore()->GetCmdStack()->RemoveLastUndo();

                Q3DStudio::ScopedDocumentEditor sceneEditor(
                            Q3DStudio::SCOPED_DOCUMENT_EDITOR(
                                *doc, tr("Drag and Drop Material")));
                QString docDir = doc->GetDocumentDirectory();
                QString relPath = Q3DStudio::CFilePath::GetRelativePathFromBase(
                            docDir, m_objectRequestData.m_matFilePath);
                sceneEditor->SetMaterialType(matInstance, QStringLiteral("Referenced Material"));
                sceneEditor->setMaterialSourcePath(matInstance, relPath);
                sceneEditor->setMaterialReferenceByPath(matInstance, relPath);
                doc->SelectDataModelObject(matInstance);
            } else {
                g_StudioApp.GetDialogs()->DisplayMessageBox(
                            tr("Error"), tr("The character '#' is not allowed in "
                                            "the name of a material definition file."),
                            Qt3DSMessageBox::ICON_ERROR, false);
            }
        }
    }
}

QSize Q3DSPlayerWnd::sizeHint() const
{
    return effectivePresentationSize();
}

void Q3DSPlayerWnd::onDragEnter()
{
    m_objectRequestData.clear();
}

//==============================================================================
/**
 * SetPlayerWndPosition: Sets the position of the child player window
 *
 * Called when the view is scrolled to position the child player window
 *
 */
//==============================================================================
void Q3DSPlayerWnd::setWindowPosition()
{
    recenterClient();
}

//==============================================================================
/**
 *  SetScrollRanges: Sets the scroll ranges when the view is being resized
 */
//==============================================================================
void Q3DSPlayerWnd::setScrollRanges()
{
    long theScrollWidth = 0;
    long theScrollHeight = 0;

    if (shouldHideScrollBars()) {
        horizontalScrollBar()->setRange(0, 0);
        verticalScrollBar()->setRange(0, 0);
        horizontalScrollBar()->setValue(0);
        verticalScrollBar()->setValue(0);
    } else {
        QSize theSize = effectivePresentationSize();

        theScrollWidth = theSize.width();
        theScrollHeight = theSize.height();

        // Set scrollbar ranges
        horizontalScrollBar()->setRange(0, theScrollWidth - width());
        verticalScrollBar()->setRange(0, theScrollHeight - height());
        horizontalScrollBar()->setPageStep(width());
        verticalScrollBar()->setPageStep(height());
        horizontalScrollBar()->setVisible(true);
        verticalScrollBar()->setVisible(true);
    }

    // Setting scroll ranges will do some async geometry adjustments, so do the
    // recentering asynchronously as well
    QTimer::singleShot(0, [this]() {
        recenterClient();
    });
}


//==============================================================================
/**
 *  RecenterClient: Recenters the Client rect in the View's client area.
 */
//==============================================================================
void Q3DSPlayerWnd::recenterClient()
{
    QRect theViewRect = rect();
    QSize theClientSize;
    QSize viewSize;
    m_ClientRect = theViewRect;
    viewSize = theViewRect.size();

    if (!shouldHideScrollBars()) {
        theClientSize = effectivePresentationSize();

        if (theClientSize.width() < theViewRect.width()) {
            m_ClientRect.setLeft(
                    even((theViewRect.width() / 2) - (theClientSize.width() / 2)));
        } else {
            m_ClientRect.setLeft(-horizontalScrollBar()->value());
        }
        m_ClientRect.setWidth(theClientSize.width());

        if (theClientSize.height() < theViewRect.height()) {
            m_ClientRect.setTop(
                    even((theViewRect.height() / 2) - (theClientSize.height() / 2)));
        } else {
            m_ClientRect.setTop(-verticalScrollBar()->value());
        }
        m_ClientRect.setHeight(theClientSize.height());
    }

    QRect glRect = m_ClientRect;
    const qreal pixelRatio = StudioUtils::devicePixelRatio(window()->windowHandle());
    glRect.setX(m_ClientRect.left() * pixelRatio);
    glRect.setY(m_ClientRect.top() * pixelRatio);
    glRect.setWidth(int(pixelRatio * m_ClientRect.width()));
    glRect.setHeight(int(pixelRatio * m_ClientRect.height()));
    m_widget->setGeometry(m_ClientRect);
    g_StudioApp.getRenderer().SetViewRect(glRect, glRect.size());
}

//==============================================================================
/**
 *  OnRulerGuideToggled:
 *  Handle scrollbar position when ruler, guide has been toggled
 */
//==============================================================================
void Q3DSPlayerWnd::onRulerGuideToggled()
{
    int scrollAmount = g_StudioApp.getRenderer().AreGuidesEnabled() ? 16 : -16;
    bool hasHorz = horizontalScrollBar()->isVisible();
    bool hasVert = verticalScrollBar()->isVisible();
    int hscrollPos = 0, vscrollPos = 0;
    if (hasHorz)
        hscrollPos = qMax(horizontalScrollBar()->value() + scrollAmount, 0);
    if (hasVert)
        vscrollPos = qMax(verticalScrollBar()->value() + scrollAmount, 0);
    horizontalScrollBar()->setValue(hscrollPos);
    verticalScrollBar()->setValue(vscrollPos);
    m_widget->update();
}

//==============================================================================
/**
 *  Set the view mode of the current scene view, whether we are in editing mode
 *  or deployment mode. For editing mode, we want to use the full scene area without
 *  any matte area.
 *  @param inViewMode  the view mode of this scene
 */
void Q3DSPlayerWnd::setViewMode(EViewMode inViewMode)
{
    m_ViewMode = inViewMode;
    m_SceneView->recheckSizingMode();
}

//==============================================================================
/**
 *  Checks whether we are in deployment view mode.
 *  @return true if is in deployment view mode, else false
 */
bool Q3DSPlayerWnd::isDeploymentView()
{
    return m_ViewMode == VIEW_SCENE ? true : false;
}

QSize Q3DSPlayerWnd::effectivePresentationSize() const
{
    QSize theSize = g_StudioApp.GetCore()->GetStudioProjectSettings()->getPresentationSize();

    // If we have guides, resize the window with enough space for the guides as well as the
    // presentation
    // This is a very dirty hack because we are of course hardcoding the size of the guides.
    // If the size of the guides never changes, the bet paid off.
    // TODO: redo for guide rendering
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    if (g_StudioApp.getRenderer().AreGuidesEnabled())
        theSize += QSize(CStudioPreferences::guideSize(), CStudioPreferences::guideSize());
#endif
    return theSize;
}

void Q3DSPlayerWnd::wheelEvent(QWheelEvent* event)
{
    const bool theCtrlKeyIsDown = event->modifiers() & Qt::ControlModifier;

    if (!theCtrlKeyIsDown && !isDeploymentView()) {
        // Zoom when in edit camera view
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseWheel(
                    SceneDragSenderType::Matte, event->delta(), STUDIO_TOOLMODE_CAMERA_ZOOM);
    } else {
        // Otherwise, scroll the view
        QScrollArea::wheelEvent(event);
    }
}

void Q3DSPlayerWnd::scrollContentsBy(int, int)
{
    setWindowPosition();
}

bool Q3DSPlayerWnd::shouldHideScrollBars()
{
    return m_ViewMode == VIEW_EDIT || g_StudioApp.IsAuthorZoom();
}

}
