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
#include "WebProcessCreationParameters.h"

#include "ArgumentCoders.h"

namespace WebKit {

WebProcessCreationParameters::WebProcessCreationParameters()
    : shouldTrackVisitedLinks(false)
    , clearResourceCaches(false)
    , clearApplicationCache(false)
    , shouldAlwaysUseComplexTextCodePath(false)
    , defaultRequestTimeoutInterval(INT_MAX)
#if PLATFORM(MAC)
    , nsURLCacheMemoryCapacity(0)
    , nsURLCacheDiskCapacity(0)
#elif PLATFORM(WIN)
    , shouldPaintNativeControls(false)
#endif
{
}

void WebProcessCreationParameters::encode(CoreIPC::ArgumentEncoder* encoder) const
{
    encoder->encode(injectedBundlePath);
    encoder->encode(injectedBundlePathExtensionHandle);
    encoder->encode(applicationCacheDirectory);
    encoder->encode(databaseDirectory);
    encoder->encode(localStorageDirectory);
    encoder->encode(urlSchemesRegistererdAsEmptyDocument);
    encoder->encode(urlSchemesRegisteredAsSecure);
    encoder->encode(urlSchemesForWhichDomainRelaxationIsForbidden);
    encoder->encode(mimeTypesWithCustomRepresentation);
    encoder->encodeEnum(cacheModel);
    encoder->encode(shouldTrackVisitedLinks);
    encoder->encode(clearResourceCaches);
    encoder->encode(clearApplicationCache);
    encoder->encode(shouldAlwaysUseComplexTextCodePath);
    encoder->encode(iconDatabaseEnabled);
    encoder->encode(languageCode);
    encoder->encode(textCheckerState);
    encoder->encode(defaultRequestTimeoutInterval);
#if USE(CFURLSTORAGESESSIONS)
    encoder->encode(uiProcessBundleIdentifier);
#endif
#if PLATFORM(MAC)
    encoder->encode(parentProcessName);
    encoder->encode(presenterApplicationPid);
    encoder->encode(nsURLCachePath);
    encoder->encode(nsURLCacheMemoryCapacity);
    encoder->encode(nsURLCacheDiskCapacity);
    encoder->encode(acceleratedCompositingPort);
    encoder->encode(uiProcessBundleResourcePath);
#elif PLATFORM(WIN)
    encoder->encode(shouldPaintNativeControls);
    encoder->encode(cfURLCachePath);
    encoder->encode(cfURLCacheDiskCapacity);
    encoder->encode(cfURLCacheMemoryCapacity);
    encoder->encode(initialHTTPCookieAcceptPolicy);
#endif
}

bool WebProcessCreationParameters::decode(CoreIPC::ArgumentDecoder* decoder, WebProcessCreationParameters& parameters)
{
    if (!decoder->decode(parameters.injectedBundlePath))
        return false;
    if (!decoder->decode(parameters.injectedBundlePathExtensionHandle))
        return false;
    if (!decoder->decode(parameters.applicationCacheDirectory))
        return false;
    if (!decoder->decode(parameters.databaseDirectory))
        return false;
    if (!decoder->decode(parameters.localStorageDirectory))
        return false;
    if (!decoder->decode(parameters.urlSchemesRegistererdAsEmptyDocument))
        return false;
    if (!decoder->decode(parameters.urlSchemesRegisteredAsSecure))
        return false;
    if (!decoder->decode(parameters.urlSchemesForWhichDomainRelaxationIsForbidden))
        return false;
    if (!decoder->decode(parameters.mimeTypesWithCustomRepresentation))
        return false;
    if (!decoder->decodeEnum(parameters.cacheModel))
        return false;
    if (!decoder->decode(parameters.shouldTrackVisitedLinks))
        return false;
    if (!decoder->decode(parameters.clearResourceCaches))
        return false;
    if (!decoder->decode(parameters.clearApplicationCache))
        return false;
    if (!decoder->decode(parameters.shouldAlwaysUseComplexTextCodePath))
        return false;
    if (!decoder->decode(parameters.iconDatabaseEnabled))
        return false;
    if (!decoder->decode(parameters.languageCode))
        return false;
    if (!decoder->decode(parameters.textCheckerState))
        return false;
    if (!decoder->decode(parameters.defaultRequestTimeoutInterval))
        return false;
#if USE(CFURLSTORAGESESSIONS)
    if (!decoder->decode(parameters.uiProcessBundleIdentifier))
        return false;
#endif

#if PLATFORM(MAC)
    if (!decoder->decode(parameters.parentProcessName))
        return false;
    if (!decoder->decode(parameters.presenterApplicationPid))
        return false;
    if (!decoder->decode(parameters.nsURLCachePath))
        return false;
    if (!decoder->decode(parameters.nsURLCacheMemoryCapacity))
        return false;
    if (!decoder->decode(parameters.nsURLCacheDiskCapacity))
        return false;
    if (!decoder->decode(parameters.acceleratedCompositingPort))
        return false;
    if (!decoder->decode(parameters.uiProcessBundleResourcePath))
        return false;
#elif PLATFORM(WIN)
    if (!decoder->decode(parameters.shouldPaintNativeControls))
        return false;
    if (!decoder->decode(parameters.cfURLCachePath))
        return false;
    if (!decoder->decode(parameters.cfURLCacheDiskCapacity))
        return false;
    if (!decoder->decode(parameters.cfURLCacheMemoryCapacity))
        return false;
    if (!decoder->decode(parameters.initialHTTPCookieAcceptPolicy))
        return false;
#endif

    return true;
}

} // namespace WebKit
