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
#include "WebEditorClient.h"

#include "SelectionState.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrameLoaderClient.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/ArchiveResource.h>
#include <WebCore/DocumentFragment.h>
#include <WebCore/EditCommand.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/UserTypingGestureIndicator.h>

using namespace WebCore;
using namespace HTMLNames;

namespace WebKit {

void WebEditorClient::pageDestroyed()
{
    delete this;
}

bool WebEditorClient::shouldDeleteRange(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldDeleteRange(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldShowDeleteInterface(HTMLElement*)
{
    notImplemented();
    return false;
}

bool WebEditorClient::smartInsertDeleteEnabled()
{
    // FIXME: Why isn't this Mac specific like toggleSmartInsertDeleteEnabled?
#if PLATFORM(MAC)
    return m_page->isSmartInsertDeleteEnabled();
#else
    return true;
#endif
}
 
bool WebEditorClient::isSelectTrailingWhitespaceEnabled()
{
    notImplemented();
    return false;
}

bool WebEditorClient::isContinuousSpellCheckingEnabled()
{
    return WebProcess::shared().textCheckerState().isContinuousSpellCheckingEnabled;
}

void WebEditorClient::toggleContinuousSpellChecking()
{
    notImplemented();
}

bool WebEditorClient::isGrammarCheckingEnabled()
{
    return WebProcess::shared().textCheckerState().isGrammarCheckingEnabled;
}

void WebEditorClient::toggleGrammarChecking()
{
    notImplemented();
}

int WebEditorClient::spellCheckerDocumentTag()
{
    notImplemented();
    return false;
}

    
bool WebEditorClient::isEditable()
{
    notImplemented();
    return false;
}


bool WebEditorClient::shouldBeginEditing(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldBeginEditing(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldEndEditing(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldEndEditing(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldInsertNode(Node* node, Range* rangeToReplace, EditorInsertAction action)
{
    bool result = m_page->injectedBundleEditorClient().shouldInsertNode(m_page, node, rangeToReplace, action);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldInsertText(const String& text, Range* rangeToReplace, EditorInsertAction action)
{
    bool result = m_page->injectedBundleEditorClient().shouldInsertText(m_page, text.impl(), rangeToReplace, action);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity affinity, bool stillSelecting)
{
    bool result = m_page->injectedBundleEditorClient().shouldChangeSelectedRange(m_page, fromRange, toRange, affinity, stillSelecting);
    notImplemented();
    return result;
}
    
bool WebEditorClient::shouldApplyStyle(CSSStyleDeclaration* style, Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldApplyStyle(m_page, style, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldMoveRangeAfterDelete(Range*, Range*)
{
    notImplemented();
    return true;
}

void WebEditorClient::didBeginEditing()
{
    // FIXME: What good is a notification name, if it's always the same?
    DEFINE_STATIC_LOCAL(String, WebViewDidBeginEditingNotification, ("WebViewDidBeginEditingNotification"));
    m_page->injectedBundleEditorClient().didBeginEditing(m_page, WebViewDidBeginEditingNotification.impl());
    notImplemented();
}

void WebEditorClient::respondToChangedContents()
{
    DEFINE_STATIC_LOCAL(String, WebViewDidChangeNotification, ("WebViewDidChangeNotification"));
    m_page->injectedBundleEditorClient().didChange(m_page, WebViewDidChangeNotification.impl());
    notImplemented();
}

void WebEditorClient::respondToChangedSelection()
{
    DEFINE_STATIC_LOCAL(String, WebViewDidChangeSelectionNotification, ("WebViewDidChangeSelectionNotification"));
    m_page->injectedBundleEditorClient().didChangeSelection(m_page, WebViewDidChangeSelectionNotification.impl());
    Frame* frame = m_page->corePage()->focusController()->focusedFrame();
    if (!frame)
        return;

    SelectionState selectionState;
    selectionState.isNone = frame->selection()->isNone();
    selectionState.isContentEditable = frame->selection()->isContentEditable();
    selectionState.isContentRichlyEditable = frame->selection()->isContentRichlyEditable();
    selectionState.isInPasswordField = frame->selection()->isInPasswordField();
    selectionState.hasComposition = frame->editor()->hasComposition();

    WebPage::getLocationAndLengthFromRange(frame->selection()->toNormalizedRange().get(), selectionState.selectedRangeStart, selectionState.selectedRangeLength);

    m_page->send(Messages::WebPageProxy::SelectionStateChanged(selectionState));

#if PLATFORM(WIN)
    // FIXME: This should also go into the selection state.
    if (!frame->editor()->hasComposition() || frame->editor()->ignoreCompositionSelectionChange())
        return;

    unsigned start;
    unsigned end;
    m_page->send(Messages::WebPageProxy::DidChangeCompositionSelection(frame->editor()->getCompositionSelection(start, end)));
#endif
}
    
void WebEditorClient::didEndEditing()
{
    DEFINE_STATIC_LOCAL(String, WebViewDidEndEditingNotification, ("WebViewDidEndEditingNotification"));
    m_page->injectedBundleEditorClient().didEndEditing(m_page, WebViewDidEndEditingNotification.impl());
    notImplemented();
}

void WebEditorClient::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void WebEditorClient::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

void WebEditorClient::registerCommandForUndo(PassRefPtr<EditCommand> command)
{
    // FIXME: Add assertion that the command being reapplied is the same command that is
    // being passed to us.
    if (m_page->isInRedo())
        return;

    RefPtr<WebEditCommand> webCommand = WebEditCommand::create(command);
    m_page->addWebEditCommand(webCommand->commandID(), webCommand.get());
    uint32_t editAction = static_cast<uint32_t>(webCommand->command()->editingAction());

    m_page->send(Messages::WebPageProxy::RegisterEditCommandForUndo(webCommand->commandID(), editAction));
}

void WebEditorClient::registerCommandForRedo(PassRefPtr<EditCommand>)
{
}

void WebEditorClient::clearUndoRedoOperations()
{
    m_page->send(Messages::WebPageProxy::ClearAllEditCommands());
}

bool WebEditorClient::canCopyCut(bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canPaste(bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canUndo() const
{
    notImplemented();
    return false;
}

bool WebEditorClient::canRedo() const
{
    notImplemented();
    return false;
}

void WebEditorClient::undo()
{
    notImplemented();
}

void WebEditorClient::redo()
{
    notImplemented();
}

#if !PLATFORM(MAC)
void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    if (m_page->handleEditingKeyboardEvent(event))
        event->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent*)
{
    notImplemented();
}
#endif

void WebEditorClient::textFieldDidBeginEditing(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    WebFrame* webFrame =  static_cast<WebFrameLoaderClient*>(element->document()->frame()->loader()->client())->webFrame();
    m_page->injectedBundleFormClient().textFieldDidBeginEditing(m_page, static_cast<HTMLInputElement*>(element), webFrame);
}

void WebEditorClient::textFieldDidEndEditing(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    WebFrame* webFrame =  static_cast<WebFrameLoaderClient*>(element->document()->frame()->loader()->client())->webFrame();
    m_page->injectedBundleFormClient().textFieldDidEndEditing(m_page, static_cast<HTMLInputElement*>(element), webFrame);
}

void WebEditorClient::textDidChangeInTextField(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    if (!UserTypingGestureIndicator::processingUserTypingGesture() || UserTypingGestureIndicator::focusedElementAtGestureStart() != element)
        return;

    WebFrame* webFrame =  static_cast<WebFrameLoaderClient*>(element->document()->frame()->loader()->client())->webFrame();
    m_page->injectedBundleFormClient().textDidChangeInTextField(m_page, static_cast<HTMLInputElement*>(element), webFrame);
}

void WebEditorClient::textDidChangeInTextArea(Element* element)
{
    if (!element->hasTagName(textareaTag))
        return;

    WebFrame* webFrame =  static_cast<WebFrameLoaderClient*>(element->document()->frame()->loader()->client())->webFrame();
    m_page->injectedBundleFormClient().textDidChangeInTextArea(m_page, static_cast<HTMLTextAreaElement*>(element), webFrame);
}

static bool getActionTypeForKeyEvent(KeyboardEvent* event, WKInputFieldActionType& type)
{
    String key = event->keyIdentifier();
    if (key == "Up")
        type = WKInputFieldActionTypeMoveUp;
    else if (key == "Down")
        type = WKInputFieldActionTypeMoveDown;
    else if (key == "U+001B")
        type = WKInputFieldActionTypeCancel;
    else if (key == "U+0009") {
        if (event->shiftKey())
            type = WKInputFieldActionTypeInsertBacktab;
        else
            type = WKInputFieldActionTypeInsertTab;
    } else if (key == "Enter")
        type = WKInputFieldActionTypeInsertNewline;
    else
        return false;

    return true;
}

bool WebEditorClient::doTextFieldCommandFromEvent(Element* element, KeyboardEvent* event)
{
    if (!element->hasTagName(inputTag))
        return false;

    WKInputFieldActionType actionType = static_cast<WKInputFieldActionType>(0);
    if (!getActionTypeForKeyEvent(event, actionType))
        return false;

    WebFrame* webFrame =  static_cast<WebFrameLoaderClient*>(element->document()->frame()->loader()->client())->webFrame();
    return m_page->injectedBundleFormClient().shouldPerformActionInTextField(m_page, static_cast<HTMLInputElement*>(element), actionType, webFrame);
}

void WebEditorClient::textWillBeDeletedInTextField(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    WebFrame* webFrame =  static_cast<WebFrameLoaderClient*>(element->document()->frame()->loader()->client())->webFrame();
    m_page->injectedBundleFormClient().shouldPerformActionInTextField(m_page, static_cast<HTMLInputElement*>(element), WKInputFieldActionTypeInsertDelete, webFrame);
}

void WebEditorClient::ignoreWordInSpellDocument(const String& word)
{
    m_page->send(Messages::WebPageProxy::IgnoreWord(word));
}

void WebEditorClient::learnWord(const String& word)
{
    m_page->send(Messages::WebPageProxy::LearnWord(word));
}

void WebEditorClient::checkSpellingOfString(const UChar*, int, int*, int*)
{
    notImplemented();
}

String WebEditorClient::getAutoCorrectSuggestionForMisspelledWord(const String&)
{
    notImplemented();
    return String();
}

void WebEditorClient::checkGrammarOfString(const UChar*, int, Vector<GrammarDetail>&, int*, int*)
{
    notImplemented();
}

void WebEditorClient::updateSpellingUIWithGrammarString(const String& badGrammarPhrase, const GrammarDetail& grammarDetail)
{
    m_page->send(Messages::WebPageProxy::UpdateSpellingUIWithGrammarString(badGrammarPhrase, grammarDetail));
}

void WebEditorClient::updateSpellingUIWithMisspelledWord(const String& misspelledWord)
{
    m_page->send(Messages::WebPageProxy::UpdateSpellingUIWithMisspelledWord(misspelledWord));
}

void WebEditorClient::showSpellingUI(bool)
{
    notImplemented();
}

bool WebEditorClient::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void WebEditorClient::getGuessesForWord(const String& word, const String& context, Vector<String>& guesses)
{
    m_page->sendSync(Messages::WebPageProxy::GetGuessesForWord(word, context), Messages::WebPageProxy::GetGuessesForWord::Reply(guesses));
}

void WebEditorClient::willSetInputMethodState()
{
    notImplemented();
}

void WebEditorClient::setInputMethodState(bool)
{
    notImplemented();
}

void WebEditorClient::requestCheckingOfString(WebCore::SpellChecker*, int, const WTF::String&)
{
    notImplemented();
}

} // namespace WebKit
