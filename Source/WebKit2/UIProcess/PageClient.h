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

#ifndef PageClient_h
#define PageClient_h

#include "ShareableBitmap.h"
#include "WebPageProxy.h"
#include "WebPopupMenuProxy.h"
#include <wtf/Forward.h>

namespace WebCore {
    class Cursor;
    struct ViewportArguments;
}

namespace WebKit {

class DrawingAreaProxy;
class FindIndicator;
class NativeWebKeyboardEvent;
class NativeWebKeyboardEvent;
class WebContextMenuProxy;
class WebEditCommandProxy;
class WebPopupMenuProxy;

class PageClient {
public:
    virtual ~PageClient() { }

    // Create a new drawing area proxy for the given page.
    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy() = 0;

    // Tell the view to invalidate the given rect. The rect is in view coordinates.
    virtual void setViewNeedsDisplay(const WebCore::IntRect&) = 0;

    // Tell the view to immediately display its invalid rect.
    virtual void displayView() = 0;

    // Tell the view to scroll scrollRect by scrollOffset.
    virtual void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset) = 0;

    // Return the size of the view the page is associated with.
    virtual WebCore::IntSize viewSize() = 0;

    // Return whether the view's containing window is active.
    virtual bool isViewWindowActive() = 0;

    // Return whether the view is focused.
    virtual bool isViewFocused() = 0;

    // Return whether the view is visible.
    virtual bool isViewVisible() = 0;

    // Return whether the view is in a window.
    virtual bool isViewInWindow() = 0;
    
    virtual void processDidCrash() = 0;
    virtual void didRelaunchProcess() = 0;
    virtual void pageClosed() = 0;

    virtual void setFocus(bool focused) = 0;
    virtual void takeFocus(bool direction) = 0;
    virtual void toolTipChanged(const String&, const String&) = 0;

#if ENABLE(TILED_BACKING_STORE)
    virtual void pageDidRequestScroll(const WebCore::IntPoint&) = 0;
#endif
#if PLATFORM(QT)
    virtual void didChangeContentsSize(const WebCore::IntSize&) = 0;
    virtual void didFindZoomableArea(const WebCore::IntRect&) = 0;
#endif

    virtual void setCursor(const WebCore::Cursor&) = 0;
    virtual void setViewportArguments(const WebCore::ViewportArguments&) = 0;

    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo) = 0;
    virtual void clearAllEditCommands() = 0;
#if PLATFORM(MAC)
    virtual void accessibilityWebProcessTokenReceived(const CoreIPC::DataReference&) = 0;
    virtual bool interpretKeyEvent(const NativeWebKeyboardEvent&, const TextInputState&, Vector<WebCore::KeypressCommand>&) = 0;
    virtual void setDragImage(const WebCore::IntPoint& clientPosition, PassRefPtr<ShareableBitmap> dragImage, bool isLinkDrag) = 0;
#endif
#if PLATFORM(WIN)
    virtual void compositionSelectionChanged(bool) = 0;
#endif
    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&) = 0;
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&) = 0;
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect&) = 0;
    
    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool wasEventHandled) = 0;

    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*) = 0;
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*) = 0;

    virtual void setFindIndicator(PassRefPtr<FindIndicator>, bool fadeOut) = 0;

#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&) = 0;
    virtual void exitAcceleratedCompositingMode() = 0;
#endif

#if PLATFORM(WIN)
    virtual HWND nativeWindow() = 0;
#endif

#if PLATFORM(MAC)
    virtual void setComplexTextInputEnabled(uint64_t pluginComplexTextInputIdentifier, bool complexTextInputEnabled) = 0;
    virtual CGContextRef containingWindowGraphicsContext() = 0;
    virtual void didPerformDictionaryLookup(const String&, double scaleFactor, const DictionaryPopupInfo&) = 0;
    virtual void showCorrectionPanel(WebCore::CorrectionPanelInfo::PanelType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings) = 0;
    virtual void dismissCorrectionPanel(WebCore::ReasonForDismissingCorrectionPanel) = 0;
    virtual String dismissCorrectionPanelSoon(WebCore::ReasonForDismissingCorrectionPanel) = 0;
    virtual void recordAutocorrectionResponse(WebCore::EditorClient::AutocorrectionResponseType, const String& replacedString, const String& replacementString) = 0;
#endif

    virtual void didChangeScrollbarsForMainFrame() const = 0;

    // Custom representations.
    virtual void didCommitLoadForMainFrame(bool useCustomRepresentation) = 0;
    virtual void didFinishLoadingDataForCustomRepresentation(const String& suggestedFilename, const CoreIPC::DataReference&) = 0;
    virtual double customRepresentationZoomFactor() = 0;
    virtual void setCustomRepresentationZoomFactor(double) = 0;

    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>& updateRects) = 0;

    virtual float userSpaceScaleFactor() const = 0;
};

} // namespace WebKit

#endif // PageClient_h
