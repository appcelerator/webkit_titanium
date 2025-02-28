/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2009-2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InjectedScriptManager_h
#define InjectedScriptManager_h

#include "PlatformString.h"
#include "ScriptState.h"

#include <wtf/Forward.h>
#include <wtf/HashMap.h>

namespace WebCore {

class InjectedScript;
class InjectedScriptHost;
class InspectorObject;
class ScriptObject;

class InjectedScriptManager {
    WTF_MAKE_NONCOPYABLE(InjectedScriptManager);
public:
    static PassOwnPtr<InjectedScriptManager> create();
    ~InjectedScriptManager();

    void disconnect();

    InjectedScriptHost* injectedScriptHost();

    pair<long, ScriptObject> injectScript(const String& source, ScriptState*);
    InjectedScript injectedScriptFor(ScriptState*);
    InjectedScript injectedScriptForId(long);
    InjectedScript injectedScriptForObjectId(const String& objectId);
    void discardInjectedScripts();
    void releaseObjectGroup(const String& objectGroup);

    static bool canAccessInspectedWindow(ScriptState*);

private:
    InjectedScriptManager();

    String injectedScriptSource();
    ScriptObject createInjectedScript(const String& source, ScriptState*, long id);
    void discardInjectedScript(ScriptState*);

    long m_nextInjectedScriptId;
    typedef HashMap<long, InjectedScript> IdToInjectedScriptMap;
    IdToInjectedScriptMap m_idToInjectedScript;
    RefPtr<InjectedScriptHost> m_injectedScriptHost;
};

} // namespace WebCore

#endif // !defined(InjectedScriptManager_h)
