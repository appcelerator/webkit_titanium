/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebResourceCacheManager_h
#define WebResourceCacheManager_h

#include "Arguments.h"
#include <wtf/Noncopyable.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
class MessageID;
}

namespace WebKit {

struct SecurityOriginData;

class WebResourceCacheManager {
    WTF_MAKE_NONCOPYABLE(WebResourceCacheManager);
public:
    static WebResourceCacheManager& shared();

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

private:
    WebResourceCacheManager();
    virtual ~WebResourceCacheManager();

    // Implemented in generated WebResourceCacheManagerMessageReceiver.cpp
    void didReceiveWebResourceCacheManagerMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    void getCacheOrigins(uint64_t callbackID) const;
    void clearCacheForOrigin(SecurityOriginData origin) const;
    void clearCacheForAllOrigins() const;

#if USE(CFURLCACHE)
    static RetainPtr<CFArrayRef> cfURLCacheHostNames();
    static void clearCFURLCacheForHostNames(CFArrayRef);
#endif
};

} // namespace WebKit

#endif // WebResourceCacheManager_h
