/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JSGlobalData.h"

#include "ArgList.h"
#include "Heap.h"
#include "CommonIdentifiers.h"
#include "FunctionConstructor.h"
#include "GetterSetter.h"
#include "Interpreter.h"
#include "JSActivation.h"
#include "JSAPIValueWrapper.h"
#include "JSArray.h"
#include "JSByteArray.h"
#include "JSClassRef.h"
#include "JSFunction.h"
#include "JSLock.h"
#include "JSNotAnObject.h"
#include "JSPropertyNameIterator.h"
#include "JSStaticScopeObject.h"
#include "Lexer.h"
#include "Lookup.h"
#include "Nodes.h"
#include "Parser.h"
#include "RegExpCache.h"
#include "StrictEvalActivation.h"
#include <wtf/WTFThreadData.h>
#if ENABLE(REGEXP_TRACING)
#include "RegExp.h"
#endif


#if ENABLE(JSC_MULTIPLE_THREADS)
#include <wtf/Threading.h>
#endif

#if PLATFORM(MAC)
#include "ProfilerServer.h"
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace WTF;

namespace {

using namespace JSC;

class Recompiler {
public:
    void operator()(JSCell*);
};

inline void Recompiler::operator()(JSCell* cell)
{
    if (!cell->inherits(&JSFunction::s_info))
        return;
    JSFunction* function = asFunction(cell);
    if (function->executable()->isHostFunction())
        return;
    function->jsExecutable()->discardCode();
}

} // namespace

namespace JSC {

extern JSC_CONST_HASHTABLE HashTable arrayTable;
extern JSC_CONST_HASHTABLE HashTable jsonTable;
extern JSC_CONST_HASHTABLE HashTable dateTable;
extern JSC_CONST_HASHTABLE HashTable mathTable;
extern JSC_CONST_HASHTABLE HashTable numberTable;
extern JSC_CONST_HASHTABLE HashTable objectConstructorTable;
extern JSC_CONST_HASHTABLE HashTable regExpTable;
extern JSC_CONST_HASHTABLE HashTable regExpConstructorTable;
extern JSC_CONST_HASHTABLE HashTable stringTable;

void* JSGlobalData::jsArrayVPtr;
void* JSGlobalData::jsByteArrayVPtr;
void* JSGlobalData::jsStringVPtr;
void* JSGlobalData::jsFunctionVPtr;

void JSGlobalData::storeVPtrs()
{
    // Enough storage to fit a JSArray, JSByteArray, JSString, or JSFunction.
    // COMPILE_ASSERTS below check that this is true.
    char storage[64];

    COMPILE_ASSERT(sizeof(JSArray) <= sizeof(storage), sizeof_JSArray_must_be_less_than_storage);
    JSCell* jsArray = new (storage) JSArray(JSArray::VPtrStealingHack);
    JSGlobalData::jsArrayVPtr = jsArray->vptr();
    jsArray->~JSCell();

    COMPILE_ASSERT(sizeof(JSByteArray) <= sizeof(storage), sizeof_JSByteArray_must_be_less_than_storage);
    JSCell* jsByteArray = new (storage) JSByteArray(JSByteArray::VPtrStealingHack);
    JSGlobalData::jsByteArrayVPtr = jsByteArray->vptr();
    jsByteArray->~JSCell();

    COMPILE_ASSERT(sizeof(JSString) <= sizeof(storage), sizeof_JSString_must_be_less_than_storage);
    JSCell* jsString = new (storage) JSString(JSString::VPtrStealingHack);
    JSGlobalData::jsStringVPtr = jsString->vptr();
    jsString->~JSCell();

    COMPILE_ASSERT(sizeof(JSFunction) <= sizeof(storage), sizeof_JSFunction_must_be_less_than_storage);
    char executableStorage[sizeof(VPtrHackExecutable)];
    RefPtr<Structure> executableStructure = Structure::create(Structure::VPtrStealingHack, 0);
    JSCell* executable = new (executableStorage) VPtrHackExecutable(executableStructure.get());
    JSCell* jsFunction = new (storage) JSFunction(Structure::create(Structure::VPtrStealingHack, &JSFunction::s_info), static_cast<VPtrHackExecutable*>(executable));
    JSGlobalData::jsFunctionVPtr = jsFunction->vptr();
    executable->~JSCell();
    jsFunction->~JSCell();
}

JSGlobalData::JSGlobalData(GlobalDataType globalDataType, ThreadStackType threadStackType)
    : globalDataType(globalDataType)
    , clientData(0)
    , arrayTable(fastNew<HashTable>(JSC::arrayTable))
    , dateTable(fastNew<HashTable>(JSC::dateTable))
    , jsonTable(fastNew<HashTable>(JSC::jsonTable))
    , mathTable(fastNew<HashTable>(JSC::mathTable))
    , numberTable(fastNew<HashTable>(JSC::numberTable))
    , objectConstructorTable(fastNew<HashTable>(JSC::objectConstructorTable))
    , regExpTable(fastNew<HashTable>(JSC::regExpTable))
    , regExpConstructorTable(fastNew<HashTable>(JSC::regExpConstructorTable))
    , stringTable(fastNew<HashTable>(JSC::stringTable))
    , identifierTable(globalDataType == Default ? wtfThreadData().currentIdentifierTable() : createIdentifierTable())
    , propertyNames(new CommonIdentifiers(this))
    , emptyList(new MarkedArgumentBuffer)
    , lexer(new Lexer(this))
    , parser(new Parser)
    , interpreter(0)
    , heap(this)
    , globalObjectCount(0)
    , dynamicGlobalObject(0)
    , cachedUTCOffset(NaN)
    , maxReentryDepth(threadStackType == ThreadStackTypeSmall ? MaxSmallThreadReentryDepth : MaxLargeThreadReentryDepth)
    , m_regExpCache(new RegExpCache(this))
#if ENABLE(REGEXP_TRACING)
    , m_rtTraceList(new RTTraceList())
#endif
#ifndef NDEBUG
    , exclusiveThread(0)
#endif
{
    activationStructure = JSActivation::createStructure(*this, jsNull());
    interruptedExecutionErrorStructure = JSNonFinalObject::createStructure(*this, jsNull());
    terminatedExecutionErrorStructure = JSNonFinalObject::createStructure(*this, jsNull());
    staticScopeStructure = JSStaticScopeObject::createStructure(*this, jsNull());
    strictEvalActivationStructure = StrictEvalActivation::createStructure(*this, jsNull());
    stringStructure = JSString::createStructure(*this, jsNull());
    notAnObjectStructure = JSNotAnObject::createStructure(*this, jsNull());
    propertyNameIteratorStructure = JSPropertyNameIterator::createStructure(*this, jsNull());
    getterSetterStructure = GetterSetter::createStructure(*this, jsNull());
    apiWrapperStructure = JSAPIValueWrapper::createStructure(*this, jsNull());
    scopeChainNodeStructure = ScopeChainNode::createStructure(*this, jsNull());
    executableStructure = ExecutableBase::createStructure(*this, jsNull());
    evalExecutableStructure = EvalExecutable::createStructure(*this, jsNull());
    programExecutableStructure = ProgramExecutable::createStructure(*this, jsNull());
    functionExecutableStructure = FunctionExecutable::createStructure(*this, jsNull());
    dummyMarkableCellStructure = JSCell::createDummyStructure(*this);

    interpreter = new Interpreter(*this);
    if (globalDataType == Default)
        m_stack = wtfThreadData().stack();

#if PLATFORM(MAC)
    startProfilerServerIfNeeded();
#endif
#if ENABLE(JIT) && ENABLE(INTERPRETER)
#if USE(CF)
    CFStringRef canUseJITKey = CFStringCreateWithCString(0 , "JavaScriptCoreUseJIT", kCFStringEncodingMacRoman);
    CFBooleanRef canUseJIT = (CFBooleanRef)CFPreferencesCopyAppValue(canUseJITKey, kCFPreferencesCurrentApplication);
    if (canUseJIT) {
        m_canUseJIT = kCFBooleanTrue == canUseJIT;
        CFRelease(canUseJIT);
    } else {
      char* canUseJITString = getenv("JavaScriptCoreUseJIT");
      m_canUseJIT = !canUseJITString || atoi(canUseJITString);
    }
    CFRelease(canUseJITKey);
#elif OS(UNIX)
    char* canUseJITString = getenv("JavaScriptCoreUseJIT");
    m_canUseJIT = !canUseJITString || atoi(canUseJITString);
#else
    m_canUseJIT = true;
#endif
#endif
#if ENABLE(JIT)
#if ENABLE(INTERPRETER)
    if (m_canUseJIT)
        m_canUseJIT = executableAllocator.isValid();
#endif
    jitStubs = new JITThunks(this);
#endif
}

JSGlobalData::~JSGlobalData()
{
    // By the time this is destroyed, heap.destroy() must already have been called.

    delete interpreter;
#ifndef NDEBUG
    // Zeroing out to make the behavior more predictable when someone attempts to use a deleted instance.
    interpreter = 0;
#endif

    arrayTable->deleteTable();
    dateTable->deleteTable();
    jsonTable->deleteTable();
    mathTable->deleteTable();
    numberTable->deleteTable();
    objectConstructorTable->deleteTable();
    regExpTable->deleteTable();
    regExpConstructorTable->deleteTable();
    stringTable->deleteTable();

    fastDelete(const_cast<HashTable*>(arrayTable));
    fastDelete(const_cast<HashTable*>(dateTable));
    fastDelete(const_cast<HashTable*>(jsonTable));
    fastDelete(const_cast<HashTable*>(mathTable));
    fastDelete(const_cast<HashTable*>(numberTable));
    fastDelete(const_cast<HashTable*>(objectConstructorTable));
    fastDelete(const_cast<HashTable*>(regExpTable));
    fastDelete(const_cast<HashTable*>(regExpConstructorTable));
    fastDelete(const_cast<HashTable*>(stringTable));

    delete parser;
    delete lexer;

    deleteAllValues(opaqueJSClassData);

    delete emptyList;

    delete propertyNames;
    if (globalDataType != Default)
        deleteIdentifierTable(identifierTable);

    delete clientData;
    delete m_regExpCache;
#if ENABLE(REGEXP_TRACING)
    delete m_rtTraceList;
#endif
}

PassRefPtr<JSGlobalData> JSGlobalData::createContextGroup(ThreadStackType type)
{
    return adoptRef(new JSGlobalData(APIContextGroup, type));
}

PassRefPtr<JSGlobalData> JSGlobalData::create(ThreadStackType type)
{
    return adoptRef(new JSGlobalData(Default, type));
}

PassRefPtr<JSGlobalData> JSGlobalData::createLeaked(ThreadStackType type)
{
    Structure::startIgnoringLeaks();
    RefPtr<JSGlobalData> data = create(type);
    Structure::stopIgnoringLeaks();
    return data.release();
}

bool JSGlobalData::sharedInstanceExists()
{
    return sharedInstanceInternal();
}

JSGlobalData& JSGlobalData::sharedInstance()
{
    JSGlobalData*& instance = sharedInstanceInternal();
    if (!instance) {
        instance = adoptRef(new JSGlobalData(APIShared, ThreadStackTypeSmall)).leakRef();
#if ENABLE(JSC_MULTIPLE_THREADS)
        instance->makeUsableFromMultipleThreads();
#endif
    }
    return *instance;
}

JSGlobalData*& JSGlobalData::sharedInstanceInternal()
{
    ASSERT(JSLock::currentThreadIsHoldingLock());
    static JSGlobalData* sharedInstance;
    return sharedInstance;
}

#if ENABLE(JIT)
NativeExecutable* JSGlobalData::getHostFunction(NativeFunction function)
{
    return jitStubs->hostFunctionStub(this, function);
}
NativeExecutable* JSGlobalData::getHostFunction(NativeFunction function, ThunkGenerator generator)
{
    return jitStubs->hostFunctionStub(this, function, generator);
}
#else
NativeExecutable* JSGlobalData::getHostFunction(NativeFunction function)
{
    return NativeExecutable::create(*this, function, callHostFunctionAsConstructor);
}
#endif

JSGlobalData::ClientData::~ClientData()
{
}

void JSGlobalData::resetDateCache()
{
    cachedUTCOffset = NaN;
    dstOffsetCache.reset();
    cachedDateString = UString();
    cachedDateStringValue = NaN;
    dateInstanceCache.reset();
}

void JSGlobalData::startSampling()
{
    interpreter->startSampling();
}

void JSGlobalData::stopSampling()
{
    interpreter->stopSampling();
}

void JSGlobalData::dumpSampleData(ExecState* exec)
{
    interpreter->dumpSampleData(exec);
}

void JSGlobalData::recompileAllJSFunctions()
{
    // If JavaScript is running, it's not safe to recompile, since we'll end
    // up throwing away code that is live on the stack.
    ASSERT(!dynamicGlobalObject);
    
    Recompiler recompiler;
    heap.forEach(recompiler);
}

#if ENABLE(REGEXP_TRACING)
void JSGlobalData::addRegExpToTrace(PassRefPtr<RegExp> regExp)
{
    m_rtTraceList->add(regExp);
}

void JSGlobalData::dumpRegExpTrace()
{
    // The first RegExp object is ignored.  It is create by the RegExpPrototype ctor and not used.
    RTTraceList::iterator iter = ++m_rtTraceList->begin();
    
    if (iter != m_rtTraceList->end()) {
        printf("\nRegExp Tracing\n");
        printf("                                                            match()    matches\n");
        printf("Regular Expression                          JIT Address      calls      found\n");
        printf("----------------------------------------+----------------+----------+----------\n");
    
        unsigned reCount = 0;
    
        for (; iter != m_rtTraceList->end(); ++iter, ++reCount)
            (*iter)->printTraceData();

        printf("%d Regular Expressions\n", reCount);
    }
    
    m_rtTraceList->clear();
}
#else
void JSGlobalData::dumpRegExpTrace()
{
}
#endif

} // namespace JSC
