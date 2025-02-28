/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebEditorClient_H
#define WebEditorClient_H

#include "WebKit.h"
#include <WebCore/EditorClient.h>
#include <WebCore/TextCheckerClient.h>
#include <wtf/OwnPtr.h>

class WebView;
class WebNotification;
class WebEditorUndoTarget;

class WebEditorClient : public WebCore::EditorClient, public WebCore::TextCheckerClient {
public:
    WebEditorClient(WebView*);
    ~WebEditorClient();

    virtual void pageDestroyed();

    virtual bool isContinuousSpellCheckingEnabled();
    virtual void toggleGrammarChecking();
    virtual bool isGrammarCheckingEnabled();
    virtual void toggleContinuousSpellChecking();
    virtual int spellCheckerDocumentTag();

    virtual bool shouldBeginEditing(WebCore::Range*);
    virtual bool shouldEndEditing(WebCore::Range*);
    virtual bool shouldInsertText(const WTF::String&, WebCore::Range*, WebCore::EditorInsertAction);

    virtual void didBeginEditing();
    virtual void didEndEditing();
    virtual void didWriteSelectionToPasteboard();
    virtual void didSetSelectionTypesForPasteboard();

    virtual void respondToChangedContents();
    virtual void respondToChangedSelection();

    bool shouldShowDeleteInterface(WebCore::HTMLElement*);
    bool shouldDeleteRange(WebCore::Range*);

    bool shouldInsertNode(WebCore::Node*, WebCore::Range* replacingRange, WebCore::EditorInsertAction);
    bool shouldApplyStyle(WebCore::CSSStyleDeclaration*, WebCore::Range*);
    bool shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*);
    bool shouldChangeTypingStyle(WebCore::CSSStyleDeclaration* currentStyle, WebCore::CSSStyleDeclaration* toProposedStyle);

    void webViewDidChangeTypingStyle(WebNotification*);
    void webViewDidChangeSelection(WebNotification*);

    bool smartInsertDeleteEnabled();
    bool isSelectTrailingWhitespaceEnabled();

    void registerCommandForUndo(PassRefPtr<WebCore::EditCommand>);
    void registerCommandForRedo(PassRefPtr<WebCore::EditCommand>);
    void clearUndoRedoOperations();

    bool canCopyCut(bool defaultValue) const;
    bool canPaste(bool defaultValue) const;
    bool canUndo() const;
    bool canRedo() const;
    
    void undo();
    void redo();    
    
    virtual bool shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity, bool stillSelecting);
    virtual void textFieldDidBeginEditing(WebCore::Element*);
    virtual void textFieldDidEndEditing(WebCore::Element*);
    virtual void textDidChangeInTextField(WebCore::Element*);
    virtual bool doTextFieldCommandFromEvent(WebCore::Element*, WebCore::KeyboardEvent*);
    virtual void textWillBeDeletedInTextField(WebCore::Element* input);
    virtual void textDidChangeInTextArea(WebCore::Element*);

    void handleKeyboardEvent(WebCore::KeyboardEvent*);
    void handleInputMethodKeydown(WebCore::KeyboardEvent*);

    virtual void ignoreWordInSpellDocument(const WTF::String&);
    virtual void learnWord(const WTF::String&);
    virtual void checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength);
    virtual WTF::String getAutoCorrectSuggestionForMisspelledWord(const WTF::String&);
    virtual void checkGrammarOfString(const UChar*, int length, Vector<WebCore::GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength);
    virtual void updateSpellingUIWithGrammarString(const WTF::String&, const WebCore::GrammarDetail& detail);
    virtual void updateSpellingUIWithMisspelledWord(const WTF::String&);
    virtual void showSpellingUI(bool show);
    virtual bool spellingUIIsShowing();
    virtual void getGuessesForWord(const WTF::String& word, const WTF::String& context, WTF::Vector<WTF::String>& guesses);

    virtual void willSetInputMethodState();
    virtual void setInputMethodState(bool);
    virtual void requestCheckingOfString(WebCore::SpellChecker*, int, const WTF::String&) {}
    virtual WebCore::TextCheckerClient* textChecker() { return this; }

private:
    WebView* m_webView;
    WebEditorUndoTarget* m_undoTarget;
};

#endif // WebEditorClient_H
