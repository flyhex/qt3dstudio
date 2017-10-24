/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
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

#ifndef UICVIEWER_H
#define UICVIEWER_H

#include "qt3dsruntimeglobal.h"

#include <string>
#include <vector>
#include <QObject>

#include "Qt3DSInputDefs.h"

#include <QtGui/qsurfaceformat.h>

namespace Q3DStudio {
class CTegraApplication;
class IWindowSystem;
class IAudioPlayer;
}

namespace qt3ds {
class Qt3DSAssetVisitor
{
public:
    virtual void visit(const char *path) = 0;
    virtual void visit(const char *type, const char *id, const char *src, const char *args) = 0;
};
}

namespace Q3DSViewer {

typedef void (*qml_Function)(void *inUserData);

// not must match the NDDRuntime see
// UICQmlEngine.h
struct ViewerCallbackType
{
    enum Enum {
        CALLBACK_ON_INIT = 1,
        CALLBACK_ON_UPDATE = 2,
    };
};

struct ViewerContextType
{
    enum Enum { GL_ES2 = 0, GL_ES3, GL_ES31, GL_ES32, GL_2, GL_3_3, GL_4 };
};

struct ViewerScaleModes
{
    enum Enum {
        ExactSize = 0, // Ensure the viewport is exactly same size as application
        ScaleToFit = 1, // Resize viewport keeping aspect ratio
        ScaleToFill = 2, // Resize viewport to entire window
    };
};

struct ViewerShadeModes
{
    enum Enum {
        Shaded = 0, // Geometry is shaded only
        ShadedWireframe = 1, // Wireframe is drawn on top shaded geometry
        Wireframe = 2, // Wireframe only
    };
};

class Q3DSViewerAppImpl;
class QT3DS_RUNTIME_API Q3DSViewerApp : public QObject
{
    Q_OBJECT
private:
    Q3DSViewerApp(void *glContext, Q3DStudio::IAudioPlayer *inAudioPlayer);
    ~Q3DSViewerApp();

public:
    /**
     * @brief Initialize viewer app
     *
     * @param[in] winWidth              window width
     * @param[in] winHeight             window height
     * @param[in] format                OpenGL API version
     * @param[in] offscreenID           ID of an OpenGL FBO we should render to by  default
     * @param[in] source                string to presentation source
     *
     * @return true on success
     */
    bool InitializeApp(int winWidth, int winHeight, const QSurfaceFormat& format,
                       int offscreenID, const QString &source,
                       qt3ds::Qt3DSAssetVisitor *assetVisitor = nullptr);

    bool IsInitialised(void);
    void setOffscreenId(int offscreenID);


    /*
     * @brief handle window resize
     *
     * @param[in] width		new width
     * @param[in] height	new size
     *
     * @return no return
     */
    void Resize(int width, int height);

    /*
     * @brief does the actual scene rendering
     *
     * @return no return
     */
    void Render();

    /*
     * @brief handle keyboard input
     *
     * @param[in] keyEvent		event
     * @param[in] isPressed		true if key pressed
     *
     * @return no return
     */
    void HandleKeyInput(Q3DStudio::EKeyCode inKeyCode, bool isPressed);

    /*
     * @brief handle mouse move input
     *
     * @param[in] x				mouse pos x
     * @param[in] y				mouse pos y
     * @param[in] mouseButton	mouse button id
     * @param[in] isPressed		true if key pressed
     *
     * @return no return
     */
    void HandleMouseMove(int x, int y, bool isPressed);

    /*
     * @brief handle mouse press events
     *
     * @param[in] x				mouse pos x
     * @param[in] y				mouse pos y
     * @param[in] mouseButton	mouse button id
     * @param[in] isPressed		true if key pressed
     *
     * @return no return
     */
    void HandleMousePress(int x, int y, int mouseButton, bool isPressed);

    /*
     * @brief handle mouse wheel events
     *
     * @param[in] x				mouse pos x
     * @param[in] y				mouse pos y
     * @param[in] orientation	0 vertical, 1 horizontal
     * @param[in] numSteps		how much the wheel has turned
     *
     * @return no return
     */
    void HandleMouseWheel(int x, int y, int orientation, int numSteps);

    /*
     * @brief Scale scene to adjsut to window size regarding width and height
     *
     * @param[in] inScale	true if scale false if unscale
     *
     * @return no return
     */
    void SetScaleMode(ViewerScaleModes::Enum inScaleMode);

    ViewerScaleModes::Enum GetScaleMode();

    void setMatteColor(const QColor &color);
    void setShowOnScreenStats(bool s);

    /*
     * @brief Set the shading mode
     *
     * @param[in] inShadeMode	the selected shade mode
     *
     * @return no return
     */
    void SetShadeMode(ViewerShadeModes::Enum inShadeMode);

    /*
     * @brief Save current render state
     *
     * @return no return
     */
    void SaveState();
    /*
     * @brief Restore current render state
     *
     * @return no return
     */
    void RestoreState();

    /// event call back slots

    /*
    * @brief Go to a slide by name
    *
    * @param[in] elementPath	where to find the element
    * @param[in] slideName		slide name
    *
    * @return no return
    */
    void GoToSlideByName(const char *elementPath, const char *slideName);

    /*
    * @brief Go to a slide by index
    *
    * @param[in] elementPath	where to find the element
    * @param[in] slideIndex		slide index
    *
    * @return no return
    */
    void GoToSlideByIndex(const char *elementPath, const int slideIndex);

    /*
    * @brief Go to relative slide
    *
    * @param[in] elementPath    where to find the element
    * @param[in] next           next or previous
    * @param[in] wrap           wrap to beginning if end is reached
    *
    * @return no return
    */
    void GoToSlideRelative(const char *elementPath, const bool next, const bool wrap);

    bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                      QString &currentName, QString &previousName);

    /*
    * @brief Set presentation active or inactive
    *
    * @param[in] presId         presentation id (name without .uip)
    * @param[in] active         set active or inactive
    *
    * @return no return
    */
    void SetPresentationActive(const char *presId, const bool active);

    /*
    * @brief Go to a certain time
    *
    * @param[in] elementPath	where to find the element
    * @param[in] time			time to move
    *
    * @return no return
    */
    void GoToTime(const char *elementPath, const float time);

    /*
    * @brief Play a sound
    *
    * @param[in] soundPath      sound file to play
    *
    * @return no return
    */
    void PlaySoundFile(const char *soundPath);

    /*
    * @brief Set attribute values
    *
    * @param[in] elementPath	where to find the element
    * @param[in] attributeName	attribute name
    * @param[in] value			new value of the attribute
    *
    * @return no return
    */
    void SetAttribute(const char *elementPath, const char *attributeName, const char *value);

    /*
    * @brief Get attribute values
    *
    * @param[in] elementPath	where to find the element
    * @param[in] attributeName	attribute name
    * @param[out] value			current attribute values
    *
    * @return true if successful
    */
    bool GetAttribute(const char *elementPath, const char *attributeName, void *value);

    /*
    * @brief Fire an event
    *
    * @param[in] elementPath    where to find the element to fire the event on
    * @param[in] evtName        the string that identifies and defines the event
    *
    * @return no return
    */
    void FireEvent(const char *elementPath, const char *evtName);

    /*
    * @brief Peek a custom action from the queue
    *		 Note the action is remvoved from the queue
    *
    * @param[out] outElementPath	element path
    * @param[out] actionName		action name
    *
    * @return true if action available
    */
    bool PeekCustomAction(std::string &outElementPath, std::string &actionName);

    /**
    * @brief Register a callback
    *
    * @param[in] callbackType		callback type
    * @param[in] inCallback		    pointer to callback
    * @param[in] inCallback		    pointer to user data
    *
    * @return  true on success
    */
    bool RegisterScriptCallback(ViewerCallbackType::Enum callbackType,
                                const qml_Function inCallback, void *inUserData);

    void EnableWatermarkDDS(const unsigned char *inData, size_t inDataSize);

    void SetWatermarkLocation(float x, float y);

    bool WasLastFrameDirty();

    int GetWindowHeight();
    int GetWindowWidth();

    void SetGlobalAnimationTime(qint64 inMilliSecs);

private:
    /*
     * @brief parse command line arguments this fills in our
     *		  global command line variables
     *
     * @param[in] cmdLineArgs	string to command line parameter
     *
     * @return no return
     */
    void handleCmdLineArguments(std::vector<std::string> &cmdLineArgs);
    /*
     * @brief setup search path.
     *		  This functions setup additonal search path for resoruces
     *		  and the lua engine
     *
     * @param[in] cmdLineArgs	string to command line parameter
     *
     * @return no return
     */
    void setupSearchPath(std::vector<std::string> &cmdLineArgs);

    void DoEnableWatermark();

    void DoSetWatermarkLocation();

private:
    Q3DSViewerAppImpl &m_Impl;

public:
    static Q3DSViewerApp &Create(void *glContext, Q3DStudio::IAudioPlayer *inAudioPlayer = 0);
    void Release();

Q_SIGNALS:
    void SigSlideEntered(const QString &elementPath, unsigned int index, const QString &name);
    void SigSlideExited(const QString &elementPath, unsigned int index, const QString &name);
    void SigPresentationReady();
};

} // end namespace

#endif // UICVIEWER_H
