/*
 * Copyright (C) 2011 Google, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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

#ifndef ContentSecurityPolicy_h
#define ContentSecurityPolicy_h

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSPDirective;
class KURL;
class SecurityOrigin;

class ContentSecurityPolicy : public RefCounted<ContentSecurityPolicy> {
public:
    static PassRefPtr<ContentSecurityPolicy> create(SecurityOrigin* origin = 0)
    {
        return adoptRef(new ContentSecurityPolicy(origin));
    }
    ~ContentSecurityPolicy();

    void didReceiveHeader(const String&);

    bool allowJavaScriptURLs() const;
    bool allowInlineEventHandlers() const;
    bool allowScriptFromSource(const KURL&) const;

private:
    explicit ContentSecurityPolicy(SecurityOrigin*);

    void parse(const String&);
    bool parseDirective(const UChar* begin, const UChar* end, String& name, String& value);
    void addDirective(const String& name, const String& value);

    bool m_havePolicy;
    RefPtr<SecurityOrigin> m_origin;
    OwnPtr<CSPDirective> m_scriptSrc;
};

}

#endif
