/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetscapePlugin.h"

#include "NPRuntimeObjectMap.h"
#include "NetscapePluginStream.h"
#include "PluginController.h"
#include "ShareableBitmap.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/HTTPHeaderMap.h>
#include <WebCore/IntRect.h>
#include <WebCore/KURL.h>
#include <utility>
#include <wtf/text/CString.h>

using namespace WebCore;
using namespace std;

namespace WebKit {

// The plug-in that we're currently calling NPP_New for.
static NetscapePlugin* currentNPPNewPlugin;

PassRefPtr<NetscapePlugin> NetscapePlugin::create(PassRefPtr<NetscapePluginModule> pluginModule)
{
    if (!pluginModule)
        return 0;

    return adoptRef(new NetscapePlugin(pluginModule));
}
    
NetscapePlugin::NetscapePlugin(PassRefPtr<NetscapePluginModule> pluginModule)
    : m_pluginController(0)
    , m_nextRequestID(0)
    , m_pluginModule(pluginModule)
    , m_npWindow()
    , m_isStarted(false)
#if PLATFORM(MAC)
    , m_isWindowed(false)
#else
    , m_isWindowed(true)
#endif
    , m_isTransparent(false)
    , m_inNPPNew(false)
    , m_loadManually(false)
#if PLATFORM(MAC)
    , m_drawingModel(static_cast<NPDrawingModel>(-1))
    , m_eventModel(static_cast<NPEventModel>(-1))
    , m_currentMouseEvent(0)
    , m_pluginHasFocus(false)
    , m_windowHasFocus(false)
#ifndef NP_NO_CARBON
    , m_nullEventTimer(RunLoop::main(), this, &NetscapePlugin::nullEventTimerFired)
    , m_npCGContext()
#endif
#elif PLUGIN_ARCHITECTURE(X11)
    , m_drawable(0)
    , m_pluginDisplay(0)
#endif
{
    m_npp.ndata = this;
    m_npp.pdata = 0;
    
    m_pluginModule->incrementLoadCount();
}

NetscapePlugin::~NetscapePlugin()
{
    ASSERT(!m_isStarted);

    m_pluginModule->decrementLoadCount();
}

PassRefPtr<NetscapePlugin> NetscapePlugin::fromNPP(NPP npp)
{
    if (npp)
        return static_cast<NetscapePlugin*>(npp->ndata);

    // FIXME: Return the current NetscapePlugin here.
    ASSERT_NOT_REACHED();
    return 0;
}

void NetscapePlugin::invalidate(const NPRect* invalidRect)
{
    IntRect rect;
    
    if (!invalidRect)
        rect = IntRect(0, 0, m_frameRect.width(), m_frameRect.height());
    else
        rect = IntRect(invalidRect->left, invalidRect->top,
                       invalidRect->right - invalidRect->left, invalidRect->bottom - invalidRect->top);
    
    if (platformInvalidate(rect))
        return;

    m_pluginController->invalidate(rect);
}

const char* NetscapePlugin::userAgent(NPP npp)
{
    if (npp)
        return fromNPP(npp)->userAgent();

    if (currentNPPNewPlugin)
        return currentNPPNewPlugin->userAgent();

    return 0;
}

const char* NetscapePlugin::userAgent()
{
    if (m_userAgent.isNull()) {
        m_userAgent = m_pluginController->userAgent().utf8();
        ASSERT(!m_userAgent.isNull());
    }
    return m_userAgent.data();
}

void NetscapePlugin::loadURL(const String& method, const String& urlString, const String& target, const HTTPHeaderMap& headerFields, const Vector<uint8_t>& httpBody,
                             bool sendNotification, void* notificationData)
{
    uint64_t requestID = ++m_nextRequestID;
    
    m_pluginController->loadURL(requestID, method, urlString, target, headerFields, httpBody, allowPopups());

    if (target.isNull()) {
        // The browser is going to send the data in a stream, create a plug-in stream.
        RefPtr<NetscapePluginStream> pluginStream = NetscapePluginStream::create(this, requestID, sendNotification, notificationData);
        ASSERT(!m_streams.contains(requestID));

        m_streams.set(requestID, pluginStream.release());
        return;
    }

    if (sendNotification) {
        // Eventually we are going to get a frameDidFinishLoading or frameDidFail call for this request.
        // Keep track of the notification data so we can call NPP_URLNotify.
        ASSERT(!m_pendingURLNotifications.contains(requestID));
        m_pendingURLNotifications.set(requestID, make_pair(urlString, notificationData));
    }
}

NPError NetscapePlugin::destroyStream(NPStream* stream, NPReason reason)
{
    NetscapePluginStream* pluginStream = 0;

    for (StreamsMap::const_iterator it = m_streams.begin(), end = m_streams.end(); it != end; ++it) {
        if (it->second->npStream() == stream) {
            pluginStream = it->second.get();
            break;
        }
    }

    if (!pluginStream)
        return NPERR_INVALID_INSTANCE_ERROR;

    return pluginStream->destroy(reason);
}

void NetscapePlugin::setIsWindowed(bool isWindowed)
{
    // Once the plugin has started, it's too late to change whether the plugin is windowed or not.
    // (This is true in Firefox and Chrome, too.) Disallow setting m_isWindowed in that case to
    // keep our internal state consistent.
    if (m_isStarted)
        return;

    m_isWindowed = isWindowed;
}

void NetscapePlugin::setIsTransparent(bool isTransparent)
{
    m_isTransparent = isTransparent;
}

void NetscapePlugin::setStatusbarText(const String& statusbarText)
{
    m_pluginController->setStatusbarText(statusbarText);
}

void NetscapePlugin::setException(const String& exceptionString)
{
    // FIXME: If the plug-in is running in its own process, this needs to send a CoreIPC message instead of
    // calling the runtime object map directly.
    NPRuntimeObjectMap::setGlobalException(exceptionString);
}

bool NetscapePlugin::evaluate(NPObject* npObject, const String& scriptString, NPVariant* result)
{
    return m_pluginController->evaluate(npObject, scriptString, result, allowPopups());
}

bool NetscapePlugin::isPrivateBrowsingEnabled()
{
    return m_pluginController->isPrivateBrowsingEnabled();
}

NPObject* NetscapePlugin::windowScriptNPObject()
{
    return m_pluginController->windowScriptNPObject();
}

NPObject* NetscapePlugin::pluginElementNPObject()
{
    return m_pluginController->pluginElementNPObject();
}

void NetscapePlugin::cancelStreamLoad(NetscapePluginStream* pluginStream)
{
    if (pluginStream == m_manualStream) {
        m_pluginController->cancelManualStreamLoad();
        return;
    }

    // Ask the plug-in controller to cancel this stream load.
    m_pluginController->cancelStreamLoad(pluginStream->streamID());
}

void NetscapePlugin::removePluginStream(NetscapePluginStream* pluginStream)
{
    if (pluginStream == m_manualStream) {
        m_manualStream = 0;
        return;
    }

    ASSERT(m_streams.get(pluginStream->streamID()) == pluginStream);
    m_streams.remove(pluginStream->streamID());
}

bool NetscapePlugin::isAcceleratedCompositingEnabled()
{
#if USE(ACCELERATED_COMPOSITING)
    return m_pluginController->isAcceleratedCompositingEnabled();
#else
    return false;
#endif
}

void NetscapePlugin::pushPopupsEnabledState(bool state)
{
    m_popupEnabledStates.append(state);
}
 
void NetscapePlugin::popPopupsEnabledState()
{
    ASSERT(!m_popupEnabledStates.isEmpty());

    m_popupEnabledStates.removeLast();
}

String NetscapePlugin::proxiesForURL(const String& urlString)
{
    return m_pluginController->proxiesForURL(urlString);
}
    
String NetscapePlugin::cookiesForURL(const String& urlString)
{
    return m_pluginController->cookiesForURL(urlString);
}

void NetscapePlugin::setCookiesForURL(const String& urlString, const String& cookieString)
{
    m_pluginController->setCookiesForURL(urlString, cookieString);
}

NPError NetscapePlugin::NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* savedData)
{
    return m_pluginModule->pluginFuncs().newp(pluginType, &m_npp, mode, argc, argn, argv, savedData);
}
    
NPError NetscapePlugin::NPP_Destroy(NPSavedData** savedData)
{
    return m_pluginModule->pluginFuncs().destroy(&m_npp, savedData);
}

NPError NetscapePlugin::NPP_SetWindow(NPWindow* npWindow)
{
    return m_pluginModule->pluginFuncs().setwindow(&m_npp, npWindow);
}

NPError NetscapePlugin::NPP_NewStream(NPMIMEType mimeType, NPStream* stream, NPBool seekable, uint16_t* streamType)
{
    return m_pluginModule->pluginFuncs().newstream(&m_npp, mimeType, stream, seekable, streamType);
}

NPError NetscapePlugin::NPP_DestroyStream(NPStream* stream, NPReason reason)
{
    return m_pluginModule->pluginFuncs().destroystream(&m_npp, stream, reason);
}

void NetscapePlugin::NPP_StreamAsFile(NPStream* stream, const char* filename)
{
    return m_pluginModule->pluginFuncs().asfile(&m_npp, stream, filename);
}

int32_t NetscapePlugin::NPP_WriteReady(NPStream* stream)
{
    return m_pluginModule->pluginFuncs().writeready(&m_npp, stream);
}

int32_t NetscapePlugin::NPP_Write(NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
    return m_pluginModule->pluginFuncs().write(&m_npp, stream, offset, len, buffer);
}

int16_t NetscapePlugin::NPP_HandleEvent(void* event)
{
    return m_pluginModule->pluginFuncs().event(&m_npp, event);
}

void NetscapePlugin::NPP_URLNotify(const char* url, NPReason reason, void* notifyData)
{
    m_pluginModule->pluginFuncs().urlnotify(&m_npp, url, reason, notifyData);
}

NPError NetscapePlugin::NPP_GetValue(NPPVariable variable, void *value)
{
    if (!m_pluginModule->pluginFuncs().getvalue)
        return NPERR_GENERIC_ERROR;

    return m_pluginModule->pluginFuncs().getvalue(&m_npp, variable, value);
}

NPError NetscapePlugin::NPP_SetValue(NPNVariable variable, void *value)
{
    if (!m_pluginModule->pluginFuncs().setvalue)
        return NPERR_GENERIC_ERROR;

    return m_pluginModule->pluginFuncs().setvalue(&m_npp, variable, value);
}

void NetscapePlugin::callSetWindow()
{
#if PLUGIN_ARCHITECTURE(X11)
    // We use a backing store as the painting area for the plugin.
    m_npWindow.x = 0;
    m_npWindow.y = 0;
#else
    m_npWindow.x = m_frameRect.x();
    m_npWindow.y = m_frameRect.y();
#endif
    m_npWindow.width = m_frameRect.width();
    m_npWindow.height = m_frameRect.height();
    m_npWindow.clipRect.top = m_clipRect.y();
    m_npWindow.clipRect.left = m_clipRect.x();
    m_npWindow.clipRect.bottom = m_clipRect.maxY();
    m_npWindow.clipRect.right = m_clipRect.maxX();

    NPP_SetWindow(&m_npWindow);
}

bool NetscapePlugin::shouldLoadSrcURL()
{
    // Check if we should cancel the load
    NPBool cancelSrcStream = false;

    if (NPP_GetValue(NPPVpluginCancelSrcStream, &cancelSrcStream) != NPERR_NO_ERROR)
        return true;

    return !cancelSrcStream;
}

NetscapePluginStream* NetscapePlugin::streamFromID(uint64_t streamID)
{
    return m_streams.get(streamID).get();
}

void NetscapePlugin::stopAllStreams()
{
    Vector<RefPtr<NetscapePluginStream> > streams;
    copyValuesToVector(m_streams, streams);

    for (size_t i = 0; i < streams.size(); ++i)
        streams[i]->stop(NPRES_USER_BREAK);
}

bool NetscapePlugin::allowPopups() const
{
    if (m_pluginModule->pluginFuncs().version >= NPVERS_HAS_POPUPS_ENABLED_STATE) {
        if (!m_popupEnabledStates.isEmpty())
            return m_popupEnabledStates.last();
    }

    // FIXME: Check if the current event is a user gesture.
    // Really old versions of Flash required this for popups to work, but all newer versions
    // support NPN_PushPopupEnabledState/NPN_PopPopupEnabledState.
    return false;
}

bool NetscapePlugin::initialize(PluginController* pluginController, const Parameters& parameters)
{
    ASSERT(!m_pluginController);
    ASSERT(pluginController);

    m_pluginController = pluginController;
    
    uint16_t mode = parameters.loadManually ? NP_FULL : NP_EMBED;
    
    m_loadManually = parameters.loadManually;

    CString mimeTypeCString = parameters.mimeType.utf8();

    ASSERT(parameters.names.size() == parameters.values.size());

    Vector<CString> paramNames;
    Vector<CString> paramValues;
    for (size_t i = 0; i < parameters.names.size(); ++i) {
        paramNames.append(parameters.names[i].utf8());
        paramValues.append(parameters.values[i].utf8());
    }

    // The strings that these pointers point to are kept alive by paramNames and paramValues.
    Vector<const char*> names;
    Vector<const char*> values;
    for (size_t i = 0; i < paramNames.size(); ++i) {
        names.append(paramNames[i].data());
        values.append(paramValues[i].data());
    }

#if PLATFORM(MAC)
    if (m_pluginModule->pluginQuirks().contains(PluginQuirks::MakeTransparentIfBackgroundAttributeExists)) {
        for (size_t i = 0; i < parameters.names.size(); ++i) {
            if (equalIgnoringCase(parameters.names[i], "background")) {
                setIsTransparent(true);
                break;
            }
        }
    }
#endif

    NetscapePlugin* previousNPPNewPlugin = currentNPPNewPlugin;
    
    m_inNPPNew = true;
    currentNPPNewPlugin = this;

    NPError error = NPP_New(const_cast<char*>(mimeTypeCString.data()), mode, names.size(),
                            const_cast<char**>(names.data()), const_cast<char**>(values.data()), 0);

    m_inNPPNew = false;
    currentNPPNewPlugin = previousNPPNewPlugin;

    if (error != NPERR_NO_ERROR)
        return false;

    m_isStarted = true;

    // FIXME: This is not correct in all cases.
    m_npWindow.type = NPWindowTypeDrawable;

    if (!platformPostInitialize()) {
        destroy();
        return false;
    }

    // Load the src URL if needed.
    if (!parameters.loadManually && !parameters.url.isEmpty() && shouldLoadSrcURL())
        loadURL("GET", parameters.url.string(), String(), HTTPHeaderMap(), Vector<uint8_t>(), false, 0);
    
    return true;
}
    
void NetscapePlugin::destroy()
{
    ASSERT(m_isStarted);

    // Stop all streams.
    stopAllStreams();

    NPP_Destroy(0);

    m_isStarted = false;
    m_pluginController = 0;

    platformDestroy();
}
    
void NetscapePlugin::paint(GraphicsContext* context, const IntRect& dirtyRect)
{
    ASSERT(m_isStarted);
    
    platformPaint(context, dirtyRect);
}

PassRefPtr<ShareableBitmap> NetscapePlugin::snapshot()
{
    if (!supportsSnapshotting() || m_frameRect.isEmpty())
        return 0;

    ASSERT(m_isStarted);
    
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(m_frameRect.size(), ShareableBitmap::SupportsAlpha);
    OwnPtr<GraphicsContext> context = bitmap->createGraphicsContext();

    context->translate(-m_frameRect.x(), -m_frameRect.y());

    platformPaint(context.get(), m_frameRect, true);
    
    return bitmap.release();
}

bool NetscapePlugin::isTransparent()
{
    return m_isTransparent;
}

void NetscapePlugin::geometryDidChange(const IntRect& frameRect, const IntRect& clipRect)
{
    ASSERT(m_isStarted);

    if (m_frameRect == frameRect && m_clipRect == clipRect) {
        // Nothing to do.
        return;
    }

    m_frameRect = frameRect;
    m_clipRect = clipRect;

    platformGeometryDidChange();
    callSetWindow();
}

void NetscapePlugin::frameDidFinishLoading(uint64_t requestID)
{
    ASSERT(m_isStarted);
    
    PendingURLNotifyMap::iterator it = m_pendingURLNotifications.find(requestID);
    if (it == m_pendingURLNotifications.end())
        return;

    String url = it->second.first;
    void* notificationData = it->second.second;

    m_pendingURLNotifications.remove(it);
    
    NPP_URLNotify(url.utf8().data(), NPRES_DONE, notificationData);
}

void NetscapePlugin::frameDidFail(uint64_t requestID, bool wasCancelled)
{
    ASSERT(m_isStarted);
    
    PendingURLNotifyMap::iterator it = m_pendingURLNotifications.find(requestID);
    if (it == m_pendingURLNotifications.end())
        return;

    String url = it->second.first;
    void* notificationData = it->second.second;

    m_pendingURLNotifications.remove(it);
    
    NPP_URLNotify(url.utf8().data(), wasCancelled ? NPRES_USER_BREAK : NPRES_NETWORK_ERR, notificationData);
}

void NetscapePlugin::didEvaluateJavaScript(uint64_t requestID, const String& requestURLString, const String& result)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(requestID))
        pluginStream->sendJavaScriptStream(requestURLString, result);
}

void NetscapePlugin::streamDidReceiveResponse(uint64_t streamID, const KURL& responseURL, uint32_t streamLength, 
                                              uint32_t lastModifiedTime, const String& mimeType, const String& headers)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didReceiveResponse(responseURL, streamLength, lastModifiedTime, mimeType, headers);
}

void NetscapePlugin::streamDidReceiveData(uint64_t streamID, const char* bytes, int length)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didReceiveData(bytes, length);
}

void NetscapePlugin::streamDidFinishLoading(uint64_t streamID)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didFinishLoading();
}

void NetscapePlugin::streamDidFail(uint64_t streamID, bool wasCancelled)
{
    ASSERT(m_isStarted);
    
    if (NetscapePluginStream* pluginStream = streamFromID(streamID))
        pluginStream->didFail(wasCancelled);
}

void NetscapePlugin::manualStreamDidReceiveResponse(const KURL& responseURL, uint32_t streamLength, uint32_t lastModifiedTime, 
                                                    const String& mimeType, const String& headers)
{
    ASSERT(m_isStarted);
    ASSERT(m_loadManually);
    ASSERT(!m_manualStream);
    
    m_manualStream = NetscapePluginStream::create(this, 0, false, 0);
    m_manualStream->didReceiveResponse(responseURL, streamLength, lastModifiedTime, mimeType, headers);
}

void NetscapePlugin::manualStreamDidReceiveData(const char* bytes, int length)
{
    ASSERT(m_isStarted);
    ASSERT(m_loadManually);
    ASSERT(m_manualStream);

    m_manualStream->didReceiveData(bytes, length);
}

void NetscapePlugin::manualStreamDidFinishLoading()
{
    ASSERT(m_isStarted);
    ASSERT(m_loadManually);
    ASSERT(m_manualStream);

    m_manualStream->didFinishLoading();
}

void NetscapePlugin::manualStreamDidFail(bool wasCancelled)
{
    ASSERT(m_isStarted);
    ASSERT(m_loadManually);
    ASSERT(m_manualStream);

    m_manualStream->didFail(wasCancelled);
}

bool NetscapePlugin::handleMouseEvent(const WebMouseEvent& mouseEvent)
{
    ASSERT(m_isStarted);
    
    return platformHandleMouseEvent(mouseEvent);
}
    
bool NetscapePlugin::handleWheelEvent(const WebWheelEvent& wheelEvent)
{
    ASSERT(m_isStarted);

    return platformHandleWheelEvent(wheelEvent);
}

bool NetscapePlugin::handleMouseEnterEvent(const WebMouseEvent& mouseEvent)
{
    ASSERT(m_isStarted);

    return platformHandleMouseEnterEvent(mouseEvent);
}

bool NetscapePlugin::handleMouseLeaveEvent(const WebMouseEvent& mouseEvent)
{
    ASSERT(m_isStarted);

    return platformHandleMouseLeaveEvent(mouseEvent);
}

bool NetscapePlugin::handleKeyboardEvent(const WebKeyboardEvent& keyboardEvent)
{
    ASSERT(m_isStarted);

    return platformHandleKeyboardEvent(keyboardEvent);
}

void NetscapePlugin::setFocus(bool hasFocus)
{
    ASSERT(m_isStarted);

    platformSetFocus(hasFocus);
}

NPObject* NetscapePlugin::pluginScriptableNPObject()
{
    ASSERT(m_isStarted);
    NPObject* scriptableNPObject = 0;
    
    if (NPP_GetValue(NPPVpluginScriptableNPObject, &scriptableNPObject) != NPERR_NO_ERROR)
        return 0;
    
    return scriptableNPObject;
}

void NetscapePlugin::privateBrowsingStateChanged(bool privateBrowsingEnabled)
{
    ASSERT(m_isStarted);

    // From https://wiki.mozilla.org/Plugins:PrivateMode
    //   When the browser turns private mode on or off it will call NPP_SetValue for "NPNVprivateModeBool" 
    //   (assigned enum value 18) with a pointer to an NPBool value on all applicable instances.
    //   Plugins should check the boolean value pointed to, not the pointer itself. 
    //   The value will be true when private mode is on.
    NPBool value = privateBrowsingEnabled;
    NPP_SetValue(NPNVprivateModeBool, &value);
}

bool NetscapePlugin::supportsSnapshotting() const
{
#if PLATFORM(MAC)
    return m_pluginModule && m_pluginModule->pluginQuirks().contains(PluginQuirks::SupportsSnapshotting);
#endif
    return false;
}

PluginController* NetscapePlugin::controller()
{
    return m_pluginController;
}

} // namespace WebKit
