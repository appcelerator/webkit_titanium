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
#include "InjectedBundle.h"

#include "Arguments.h"
#include "ImmutableArray.h"
#include "InjectedBundleMessageKinds.h"
#include "InjectedBundleScriptWorld.h"
#include "InjectedBundleUserMessageCoders.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WebContextMessageKinds.h"
#include "WebCoreArgumentCoders.h"
#include "WebDatabaseManager.h"
#include "WebPage.h"
#include "WebPreferencesStore.h"
#include "WebProcess.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSLock.h>
#include <WebCore/GCController.h>
#include <WebCore/JSDOMWindow.h>
#include <WebCore/Page.h>
#include <WebCore/PageGroup.h>
#include <WebCore/Settings.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>

using namespace WebCore;
using namespace JSC;

namespace WebKit {

InjectedBundle::InjectedBundle(const String& path)
    : m_path(path)
    , m_platformBundle(0)
{
    initializeClient(0);
}

InjectedBundle::~InjectedBundle()
{
}

void InjectedBundle::initializeClient(WKBundleClient* client)
{
    m_client.initialize(client);
}

void InjectedBundle::postMessage(const String& messageName, APIObject* messageBody)
{
    WebProcess::shared().connection()->deprecatedSend(WebContextLegacyMessage::PostMessage, 0, CoreIPC::In(messageName, InjectedBundleUserMessageEncoder(messageBody)));
}

void InjectedBundle::postSynchronousMessage(const String& messageName, APIObject* messageBody, RefPtr<APIObject>& returnData)
{
    RefPtr<APIObject> returnDataTmp;
    InjectedBundleUserMessageDecoder messageDecoder(returnDataTmp);
    
    bool succeeded = WebProcess::shared().connection()->deprecatedSendSync(WebContextLegacyMessage::PostSynchronousMessage, 0, CoreIPC::In(messageName, InjectedBundleUserMessageEncoder(messageBody)), CoreIPC::Out(messageDecoder));

    if (!succeeded)
        return;

    returnData = returnDataTmp;
}

void InjectedBundle::setShouldTrackVisitedLinks(bool shouldTrackVisitedLinks)
{
    PageGroup::setShouldTrackVisitedLinks(shouldTrackVisitedLinks);
}

void InjectedBundle::removeAllVisitedLinks()
{
    PageGroup::removeAllVisitedLinks();
}

void InjectedBundle::overrideXSSAuditorEnabledForTestRunner(WebPageGroupProxy* pageGroup, bool enabled)
{
    // Override the preference for all future pages.
    WebPreferencesStore::overrideXSSAuditorEnabledForTestRunner(enabled);

    // Change the setting for existing ones.
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setXSSAuditorEnabled(enabled);
}

void InjectedBundle::overrideAllowUniversalAccessFromFileURLsForTestRunner(WebPageGroupProxy* pageGroup, bool enabled)
{
    // Override the preference for all future pages.
    WebPreferencesStore::overrideAllowUniversalAccessFromFileURLsForTestRunner(enabled);

    // Change the setting for existing ones.
    const HashSet<Page*>& pages = PageGroup::pageGroup(pageGroup->identifier())->pages();
    for (HashSet<Page*>::iterator iter = pages.begin(); iter != pages.end(); ++iter)
        (*iter)->settings()->setAllowUniversalAccessFromFileURLs(enabled);
}

void InjectedBundle::clearAllDatabases()
{
    WebDatabaseManager::shared().deleteAllDatabases();
}

void InjectedBundle::setDatabaseQuota(uint64_t quota)
{
    WebDatabaseManager::shared().setQuotaForOrigin("file:///", quota);
}

static PassOwnPtr<Vector<String> > toStringVector(ImmutableArray* patterns)
{
    if (!patterns)
        return 0;

    size_t size =  patterns->size();
    if (!size)
        return 0;

    Vector<String>* patternsVector = new Vector<String>;
    patternsVector->reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i) {
        WebString* entry = patterns->at<WebString>(i);
        if (entry)
            patternsVector->uncheckedAppend(entry->string());
    }
    return patternsVector;
}

void InjectedBundle::addUserScript(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& source, const String& url, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserScriptInjectionTime injectionTime, WebCore::UserContentInjectedFrames injectedFrames)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->addUserScriptToWorld(scriptWorld->coreWorld(), source, KURL(KURL(), url), toStringVector(whitelist), toStringVector(blacklist), injectionTime, injectedFrames);
}

void InjectedBundle::addUserStyleSheet(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& source, const String& url, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserContentInjectedFrames injectedFrames)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->addUserStyleSheetToWorld(scriptWorld->coreWorld(), source, KURL(KURL(), url), toStringVector(whitelist), toStringVector(blacklist), injectedFrames);
}

void InjectedBundle::removeUserScript(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& url)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->removeUserScriptFromWorld(scriptWorld->coreWorld(), KURL(KURL(), url));
}

void InjectedBundle::removeUserStyleSheet(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld, const String& url)
{
    // url is not from KURL::string(), i.e. it has not already been parsed by KURL, so we have to use the relative URL constructor for KURL instead of the ParsedURLStringTag version.
    PageGroup::pageGroup(pageGroup->identifier())->removeUserStyleSheetFromWorld(scriptWorld->coreWorld(), KURL(KURL(), url));
}

void InjectedBundle::removeUserScripts(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld)
{
    PageGroup::pageGroup(pageGroup->identifier())->removeUserScriptsFromWorld(scriptWorld->coreWorld());
}

void InjectedBundle::removeUserStyleSheets(WebPageGroupProxy* pageGroup, InjectedBundleScriptWorld* scriptWorld)
{
    PageGroup::pageGroup(pageGroup->identifier())->removeUserStyleSheetsFromWorld(scriptWorld->coreWorld());
}

void InjectedBundle::removeAllUserContent(WebPageGroupProxy* pageGroup)
{
    PageGroup::pageGroup(pageGroup->identifier())->removeAllUserContent();
}

void InjectedBundle::garbageCollectJavaScriptObjects()
{
    gcController().garbageCollectNow();
}

void InjectedBundle::garbageCollectJavaScriptObjectsOnAlternateThreadForDebugging(bool waitUntilDone)
{
    gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

size_t InjectedBundle::javaScriptObjectsCount()
{
    JSLock lock(SilenceAssertionsOnly);
    return JSDOMWindow::commonJSGlobalData()->heap.objectCount();
}

void InjectedBundle::reportException(JSContextRef context, JSValueRef exception)
{
    if (!context || !exception)
        return;

    JSLock lock(JSC::SilenceAssertionsOnly);
    JSC::ExecState* execState = toJS(context);

    // Make sure the context has a DOMWindow global object, otherwise this context didn't originate from a Page.
    if (!toJSDOMWindow(execState->lexicalGlobalObject()))
        return;

    WebCore::reportException(execState, toJS(execState, exception));
}

void InjectedBundle::didCreatePage(WebPage* page)
{
    m_client.didCreatePage(this, page);
}

void InjectedBundle::willDestroyPage(WebPage* page)
{
    m_client.willDestroyPage(this, page);
}

void InjectedBundle::didInitializePageGroup(WebPageGroupProxy* pageGroup)
{
    m_client.didInitializePageGroup(this, pageGroup);
}

void InjectedBundle::didReceiveMessage(const String& messageName, APIObject* messageBody)
{
    m_client.didReceiveMessage(this, messageName, messageBody);
}

void InjectedBundle::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    switch (messageID.get<InjectedBundleMessage::Kind>()) {
        case InjectedBundleMessage::PostMessage: {
            String messageName;            
            RefPtr<APIObject> messageBody;
            InjectedBundleUserMessageDecoder messageDecoder(messageBody);
            if (!arguments->decode(CoreIPC::Out(messageName, messageDecoder)))
                return;

            didReceiveMessage(messageName, messageBody.get());
            return;
        }
    }

    ASSERT_NOT_REACHED();
}

} // namespace WebKit
