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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef DFGJITCodeGenerator_h
#define DFGJITCodeGenerator_h

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include <dfg/DFGGenerationInfo.h>
#include <dfg/DFGGraph.h>
#include <dfg/DFGJITCompiler.h>
#include <dfg/DFGOperations.h>
#include <dfg/DFGRegisterBank.h>

namespace JSC { namespace DFG {

class SpeculateIntegerOperand;
class SpeculateStrictInt32Operand;
class SpeculateCellOperand;


// === JITCodeGenerator ===
//
// This class provides common infrastructure used by the speculative &
// non-speculative JITs. Provides common mechanisms for virtual and
// physical register management, calls out from JIT code to helper
// functions, etc.
class JITCodeGenerator {
protected:
    typedef MacroAssembler::TrustedImm32 TrustedImm32;
    typedef MacroAssembler::Imm32 Imm32;

    // These constants are used to set priorities for spill order for
    // the register allocator.
    enum SpillOrder {
        SpillOrderNone,
        SpillOrderConstant = 1, // no spill, and cheap fill
        SpillOrderSpilled = 2,  // no spill
        SpillOrderArgument = 3, // no spill, but we may lose typeinfo
        SpillOrderJS = 4,       // needs spill
        SpillOrderCell = 4,     // needs spill
        SpillOrderInteger = 5,  // needs spill and box
        SpillOrderDouble = 6,   // needs spill and convert
        SpillOrderMax
    };


public:
    GPRReg fillInteger(NodeIndex, DataFormat& returnFormat);
    FPRReg fillDouble(NodeIndex);
    GPRReg fillJSValue(NodeIndex);

    // lock and unlock GPR & FPR registers.
    void lock(GPRReg reg)
    {
        m_gprs.lock(reg);
    }
    void lock(FPRReg reg)
    {
        m_fprs.lock(reg);
    }
    void unlock(GPRReg reg)
    {
        m_gprs.unlock(reg);
    }
    void unlock(FPRReg reg)
    {
        m_fprs.unlock(reg);
    }

    // Used to check whether a child node is on its last use,
    // and its machine registers may be reused.
    bool canReuse(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = m_jit.graph()[nodeIndex].virtualRegister;
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.canReuse();
    }
    GPRReg reuse(GPRReg reg)
    {
        m_gprs.lock(reg);
        return reg;
    }
    FPRReg reuse(FPRReg reg)
    {
        m_fprs.lock(reg);
        return reg;
    }

    // Allocate a gpr/fpr.
    GPRReg allocate()
    {
        VirtualRegister spillMe;
        GPRReg gpr = m_gprs.allocate(spillMe);
        if (spillMe != InvalidVirtualRegister)
            spill(spillMe);
        return gpr;
    }
    FPRReg fprAllocate()
    {
        VirtualRegister spillMe;
        FPRReg fpr = m_fprs.allocate(spillMe);
        if (spillMe != InvalidVirtualRegister)
            spill(spillMe);
        return fpr;
    }

    // Check whether a VirtualRegsiter is currently in a machine register.
    // We use this when filling operands to fill those that are already in
    // machine registers first (by locking VirtualRegsiters that are already
    // in machine register before filling those that are not we attempt to
    // avoid spilling values we will need immediately).
    bool isFilled(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = m_jit.graph()[nodeIndex].virtualRegister;
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.registerFormat() != DataFormatNone;
    }
    bool isFilledDouble(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = m_jit.graph()[nodeIndex].virtualRegister;
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.registerFormat() == DataFormatDouble;
    }

protected:
    JITCodeGenerator(JITCompiler& jit, bool isSpeculative)
        : m_jit(jit)
        , m_isSpeculative(isSpeculative)
        , m_compileIndex(0)
        , m_generationInfo(m_jit.codeBlock()->m_numCalleeRegisters)
    {
    }

    // These methods convert between doubles, and doubles boxed and JSValues.
    GPRReg boxDouble(FPRReg fpr, GPRReg gpr)
    {
        JITCompiler::FPRegisterID fpReg = JITCompiler::fprToRegisterID(fpr);
        JITCompiler::RegisterID reg = JITCompiler::gprToRegisterID(gpr);
        m_jit.moveDoubleToPtr(fpReg, reg);
        m_jit.subPtr(JITCompiler::tagTypeNumberRegister, reg);
        return gpr;
    }
    FPRReg unboxDouble(GPRReg gpr, FPRReg fpr)
    {
        JITCompiler::RegisterID reg = JITCompiler::gprToRegisterID(gpr);
        JITCompiler::FPRegisterID fpReg = JITCompiler::fprToRegisterID(fpr);
        m_jit.addPtr(JITCompiler::tagTypeNumberRegister, reg);
        m_jit.movePtrToDouble(reg, fpReg);
        return fpr;
    }
    GPRReg boxDouble(FPRReg fpr)
    {
        return boxDouble(fpr, allocate());
    }
    FPRReg unboxDouble(GPRReg gpr)
    {
        return unboxDouble(gpr, fprAllocate());
    }

    // Called on an operand once it has been consumed by a parent node.
    void use(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = m_jit.graph()[nodeIndex].virtualRegister;
        GenerationInfo& info = m_generationInfo[virtualRegister];

        // use() returns true when the value becomes dead, and any
        // associated resources may be freed.
        if (!info.use())
            return;

        // Release the associated machine registers.
        DataFormat registerFormat = info.registerFormat();
        if (registerFormat == DataFormatDouble)
            m_fprs.release(info.fpr());
        else if (registerFormat != DataFormatNone)
            m_gprs.release(info.gpr());
    }

    // Spill a VirtualRegister to the RegisterFile.
    void spill(VirtualRegister spillMe)
    {
        GenerationInfo& info = m_generationInfo[spillMe];

        // Check the GenerationInfo to see if this value need writing
        // to the RegisterFile - if not, mark it as spilled & return.
        if (!info.needsSpill()) {
            info.setSpilled();
            return;
        }

        DataFormat spillFormat = info.registerFormat();
        if (spillFormat == DataFormatDouble) {
            // All values are spilled as JSValues, so box the double via a temporary gpr.
            GPRReg gpr = boxDouble(info.fpr());
            m_jit.storePtr(JITCompiler::gprToRegisterID(gpr), JITCompiler::addressFor(spillMe));
            unlock(gpr);
            info.spill(DataFormatJSDouble);
            return;
        }

        // The following code handles JSValues, int32s, and cells.
        ASSERT(spillFormat == DataFormatInteger || spillFormat == DataFormatCell || spillFormat & DataFormatJS);

        JITCompiler::RegisterID reg = JITCompiler::gprToRegisterID(info.gpr());
        // We need to box int32 and cell values ...
        // but on JSVALUE64 boxing a cell is a no-op!
        if (spillFormat == DataFormatInteger)
            m_jit.orPtr(JITCompiler::tagTypeNumberRegister, reg);

        // Spill the value, and record it as spilled in its boxed form.
        m_jit.storePtr(reg, JITCompiler::addressFor(spillMe));
        info.spill((DataFormat)(spillFormat | DataFormatJS));
    }

    // Checks/accessors for constant values.
    bool isInt32Constant(NodeIndex nodeIndex) { return m_jit.isInt32Constant(nodeIndex); }
    bool isDoubleConstant(NodeIndex nodeIndex) { return m_jit.isDoubleConstant(nodeIndex); }
    bool isJSConstant(NodeIndex nodeIndex) { return m_jit.isJSConstant(nodeIndex); }
    int32_t valueOfInt32Constant(NodeIndex nodeIndex) { return m_jit.valueOfInt32Constant(nodeIndex); }
    double valueOfDoubleConstant(NodeIndex nodeIndex) { return m_jit.valueOfDoubleConstant(nodeIndex); }
    JSValue valueOfJSConstant(NodeIndex nodeIndex) { return m_jit.valueOfJSConstant(nodeIndex); }

    Identifier* identifier(unsigned index)
    {
        return &m_jit.codeBlock()->identifier(index);
    }

    // Spill all VirtualRegisters back to the RegisterFile.
    void flushRegisters()
    {
        for (GPRReg gpr = gpr0; gpr < numberOfGPRs; next(gpr)) {
            VirtualRegister name = m_gprs.name(gpr);
            if (name != InvalidVirtualRegister) {
                spill(name);
                m_gprs.release(gpr);
            }
        }
        for (FPRReg fpr = fpr0; fpr < numberOfFPRs; next(fpr)) {
            VirtualRegister name = m_fprs.name(fpr);
            if (name != InvalidVirtualRegister) {
                spill(name);
                m_fprs.release(fpr);
            }
        }
    }

#ifndef NDEBUG
    // Used to ASSERT flushRegisters() has been called prior to
    // calling out from JIT code to a C helper function.
    bool isFlushed()
    {
        for (GPRReg gpr = gpr0; gpr < numberOfGPRs; next(gpr)) {
            VirtualRegister name = m_gprs.name(gpr);
            if (name != InvalidVirtualRegister)
                return false;
        }
        for (FPRReg fpr = fpr0; fpr < numberOfFPRs; next(fpr)) {
            VirtualRegister name = m_fprs.name(fpr);
            if (name != InvalidVirtualRegister)
                return false;
        }
        return true;
    }
#endif

    // Get the JSValue representation of a constant.
    JSValue constantAsJSValue(NodeIndex nodeIndex)
    {
        Node& node = m_jit.graph()[nodeIndex];
        if (isInt32Constant(nodeIndex))
            return jsNumber(node.int32Constant());
        if (isDoubleConstant(nodeIndex))
            return JSValue(JSValue::EncodeAsDouble, node.numericConstant());
        ASSERT(isJSConstant(nodeIndex));
        return valueOfJSConstant(nodeIndex);
    }
    MacroAssembler::ImmPtr constantAsJSValueAsImmPtr(NodeIndex nodeIndex)
    {
        return MacroAssembler::ImmPtr(JSValue::encode(constantAsJSValue(nodeIndex)));
    }

    // Helper functions to enable code sharing in implementations of bit/shift ops.
    void bitOp(NodeType op, int32_t imm, MacroAssembler::RegisterID op1, MacroAssembler::RegisterID result)
    {
        switch (op) {
        case BitAnd:
            m_jit.and32(Imm32(imm), op1, result);
            break;
        case BitOr:
            m_jit.or32(Imm32(imm), op1, result);
            break;
        case BitXor:
            m_jit.xor32(Imm32(imm), op1, result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    void bitOp(NodeType op, MacroAssembler::RegisterID op1, MacroAssembler::RegisterID op2, MacroAssembler::RegisterID result)
    {
        switch (op) {
        case BitAnd:
            m_jit.and32(op1, op2, result);
            break;
        case BitOr:
            m_jit.or32(op1, op2, result);
            break;
        case BitXor:
            m_jit.xor32(op1, op2, result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    void shiftOp(NodeType op, MacroAssembler::RegisterID op1, int32_t shiftAmount, MacroAssembler::RegisterID result)
    {
        switch (op) {
        case BitRShift:
            m_jit.rshift32(op1, Imm32(shiftAmount), result);
            break;
        case BitLShift:
            m_jit.lshift32(op1, Imm32(shiftAmount), result);
            break;
        case BitURShift:
            m_jit.urshift32(op1, Imm32(shiftAmount), result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    void shiftOp(NodeType op, MacroAssembler::RegisterID op1, MacroAssembler::RegisterID shiftAmount, MacroAssembler::RegisterID result)
    {
        switch (op) {
        case BitRShift:
            m_jit.rshift32(op1, shiftAmount, result);
            break;
        case BitLShift:
            m_jit.lshift32(op1, shiftAmount, result);
            break;
        case BitURShift:
            m_jit.urshift32(op1, shiftAmount, result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }

    // Called once a node has completed code generation but prior to setting
    // its result, to free up its children. (This must happen prior to setting
    // the nodes result, since the node may have the same VirtualRegister as
    // a child, and as such will use the same GeneratioInfo).
    void useChildren(Node&);

    // These method called to initialize the the GenerationInfo
    // to describe the result of an operation.
    void integerResult(GPRReg reg, NodeIndex nodeIndex, DataFormat format = DataFormatInteger)
    {
        Node& node = m_jit.graph()[nodeIndex];
        useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister;
        GenerationInfo& info = m_generationInfo[virtualRegister];

        if (format == DataFormatInteger) {
            m_jit.jitAssertIsInt32(reg);
            m_gprs.retain(reg, virtualRegister, SpillOrderInteger);
            info.initInteger(nodeIndex, node.refCount, reg);
        } else {
            ASSERT(format == DataFormatJSInteger);
            m_jit.jitAssertIsJSInt32(reg);
            m_gprs.retain(reg, virtualRegister, SpillOrderJS);
            info.initJSValue(nodeIndex, node.refCount, reg, format);
        }
    }
    void noResult(NodeIndex nodeIndex)
    {
        Node& node = m_jit.graph()[nodeIndex];
        useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister;
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initNone(nodeIndex, node.refCount);
    }
    void cellResult(GPRReg reg, NodeIndex nodeIndex)
    {
        Node& node = m_jit.graph()[nodeIndex];
        useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister;
        m_gprs.retain(reg, virtualRegister, SpillOrderCell);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initCell(nodeIndex, node.refCount, reg);
    }
    void jsValueResult(GPRReg reg, NodeIndex nodeIndex, DataFormat format = DataFormatJS)
    {
        if (format == DataFormatJSInteger)
            m_jit.jitAssertIsJSInt32(reg);
        
        Node& node = m_jit.graph()[nodeIndex];
        useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister;
        m_gprs.retain(reg, virtualRegister, SpillOrderJS);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initJSValue(nodeIndex, node.refCount, reg, format);
    }
    void doubleResult(FPRReg reg, NodeIndex nodeIndex)
    {
        Node& node = m_jit.graph()[nodeIndex];
        useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister;
        m_fprs.retain(reg, virtualRegister, SpillOrderDouble);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initDouble(nodeIndex, node.refCount, reg);
    }
    void initConstantInfo(NodeIndex nodeIndex)
    {
        ASSERT(isInt32Constant(nodeIndex) || isDoubleConstant(nodeIndex) || isJSConstant(nodeIndex));
        Node& node = m_jit.graph()[nodeIndex];
        m_generationInfo[node.virtualRegister].initConstant(nodeIndex, node.refCount);
    }
    void initArgumentInfo(NodeIndex nodeIndex)
    {
        Node& node = m_jit.graph()[nodeIndex];
        m_generationInfo[node.virtualRegister].initArgument(nodeIndex, node.refCount);
    }

    // These methods used to sort arguments into the correct registers.
    template<GPRReg destA, GPRReg destB>
    void setupTwoStubArgs(GPRReg srcA, GPRReg srcB)
    {
        // Assuming that srcA != srcB, there are 7 interesting states the registers may be in:
        // (1) both are already in arg regs, the right way around.
        // (2) both are already in arg regs, the wrong way around.
        // (3) neither are currently in arg registers.
        // (4) srcA in in its correct reg.
        // (5) srcA in in the incorrect reg.
        // (6) srcB in in its correct reg.
        // (7) srcB in in the incorrect reg.
        //
        // The trivial approach is to simply emit two moves, to put srcA in place then srcB in
        // place (the MacroAssembler will omit redundant moves). This apporach will be safe in
        // cases 1, 3, 4, 5, 6, and in cases where srcA==srcB. The two problem cases are 2
        // (requires a swap) and 7 (must move srcB first, to avoid trampling.)

        if (srcB != destA) {
            // Handle the easy cases - two simple moves.
            m_jit.move(JITCompiler::gprToRegisterID(srcA), JITCompiler::gprToRegisterID(destA));
            m_jit.move(JITCompiler::gprToRegisterID(srcB), JITCompiler::gprToRegisterID(destB));
        } else if (srcA != destB) {
            // Handle the non-swap case - just put srcB in place first.
            m_jit.move(JITCompiler::gprToRegisterID(srcB), JITCompiler::gprToRegisterID(destB));
            m_jit.move(JITCompiler::gprToRegisterID(srcA), JITCompiler::gprToRegisterID(destA));
        } else
            m_jit.swap(JITCompiler::gprToRegisterID(destB), JITCompiler::gprToRegisterID(destB));
    }
    template<FPRReg destA, FPRReg destB>
    void setupTwoStubArgs(FPRReg srcA, FPRReg srcB)
    {
        // Assuming that srcA != srcB, there are 7 interesting states the registers may be in:
        // (1) both are already in arg regs, the right way around.
        // (2) both are already in arg regs, the wrong way around.
        // (3) neither are currently in arg registers.
        // (4) srcA in in its correct reg.
        // (5) srcA in in the incorrect reg.
        // (6) srcB in in its correct reg.
        // (7) srcB in in the incorrect reg.
        //
        // The trivial approach is to simply emit two moves, to put srcA in place then srcB in
        // place (the MacroAssembler will omit redundant moves). This apporach will be safe in
        // cases 1, 3, 4, 5, 6, and in cases where srcA==srcB. The two problem cases are 2
        // (requires a swap) and 7 (must move srcB first, to avoid trampling.)

        if (srcB != destA) {
            // Handle the easy cases - two simple moves.
            m_jit.moveDouble(JITCompiler::fprToRegisterID(srcA), JITCompiler::fprToRegisterID(destA));
            m_jit.moveDouble(JITCompiler::fprToRegisterID(srcB), JITCompiler::fprToRegisterID(destB));
            return;
        }
        
        if (srcA != destB) {
            // Handle the non-swap case - just put srcB in place first.
            m_jit.moveDouble(JITCompiler::fprToRegisterID(srcB), JITCompiler::fprToRegisterID(destB));
            m_jit.moveDouble(JITCompiler::fprToRegisterID(srcA), JITCompiler::fprToRegisterID(destA));
            return;
        }

        ASSERT(srcB == destA && srcA == destB);
        // Need to swap; pick a temporary register.
        FPRReg temp;
        if (destA != JITCompiler::argumentFPR3 && destA != JITCompiler::argumentFPR3)
            temp = JITCompiler::argumentFPR3;
        else if (destA != JITCompiler::argumentFPR2 && destA != JITCompiler::argumentFPR2)
            temp = JITCompiler::argumentFPR2;
        else {
            ASSERT(destA != JITCompiler::argumentFPR1 && destA != JITCompiler::argumentFPR1);
            temp = JITCompiler::argumentFPR1;
        }
        m_jit.moveDouble(JITCompiler::fprToRegisterID(destA), JITCompiler::fprToRegisterID(temp));
        m_jit.moveDouble(JITCompiler::fprToRegisterID(destB), JITCompiler::fprToRegisterID(destA));
        m_jit.moveDouble(JITCompiler::fprToRegisterID(temp), JITCompiler::fprToRegisterID(destB));
    }
    void setupStubArguments(GPRReg arg1, GPRReg arg2)
    {
        setupTwoStubArgs<JITCompiler::argumentGPR1, JITCompiler::argumentGPR2>(arg1, arg2);
    }
    void setupStubArguments(GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        // If neither of arg2/arg3 are in our way, then we can move arg1 into place.
        // Then we can use setupTwoStubArgs to fix arg2/arg3.
        if (arg2 != JITCompiler::argumentGPR1 && arg3 != JITCompiler::argumentGPR1) {
            m_jit.move(JITCompiler::gprToRegisterID(arg1), JITCompiler::argumentRegister1);
            setupTwoStubArgs<JITCompiler::argumentGPR2, JITCompiler::argumentGPR3>(arg2, arg3);
            return;
        }

        // If neither of arg1/arg3 are in our way, then we can move arg2 into place.
        // Then we can use setupTwoStubArgs to fix arg1/arg3.
        if (arg1 != JITCompiler::argumentGPR2 && arg3 != JITCompiler::argumentGPR2) {
            m_jit.move(JITCompiler::gprToRegisterID(arg2), JITCompiler::argumentRegister2);
            setupTwoStubArgs<JITCompiler::argumentGPR1, JITCompiler::argumentGPR3>(arg1, arg3);
            return;
        }

        // If neither of arg1/arg2 are in our way, then we can move arg3 into place.
        // Then we can use setupTwoStubArgs to fix arg1/arg2.
        if (arg1 != JITCompiler::argumentGPR3 && arg2 != JITCompiler::argumentGPR3) {
            m_jit.move(JITCompiler::gprToRegisterID(arg3), JITCompiler::argumentRegister3);
            setupTwoStubArgs<JITCompiler::argumentGPR1, JITCompiler::argumentGPR2>(arg1, arg2);
            return;
        }

        // If we get here, we haven't been able to move any of arg1/arg2/arg3.
        // Since all three are blocked, then all three must already be in the argument register.
        // But are they in the right ones?

        // First, ensure arg1 is in place.
        if (arg1 != JITCompiler::argumentGPR1) {
            m_jit.swap(JITCompiler::gprToRegisterID(arg1), JITCompiler::argumentRegister1);

            // If arg1 wasn't in argumentGPR1, one of arg2/arg3 must be.
            ASSERT(arg2 == JITCompiler::argumentGPR1 || arg3 == JITCompiler::argumentGPR1);
            // If arg2 was in argumentGPR1 it no longer is (due to the swap).
            // Otherwise arg3 must have been. Mark him as moved.
            if (arg2 == JITCompiler::argumentGPR1)
                arg2 = arg1;
            else
                arg3 = arg1;
        }

        // Either arg2 & arg3 need swapping, or we're all done.
        ASSERT((arg2 == JITCompiler::argumentGPR2 || arg3 == JITCompiler::argumentGPR3)
            || (arg2 == JITCompiler::argumentGPR3 || arg3 == JITCompiler::argumentGPR2));

        if (arg2 != JITCompiler::argumentGPR2)
            m_jit.swap(JITCompiler::argumentRegister2, JITCompiler::argumentRegister3);
    }

    // These methods add calls to C++ helper functions.
    void callOperation(J_DFGOperation_EJP operation, GPRReg result, GPRReg arg1, void* pointer)
    {
        ASSERT(isFlushed());

        m_jit.move(JITCompiler::gprToRegisterID(arg1), JITCompiler::argumentRegister1);
        m_jit.move(JITCompiler::TrustedImmPtr(pointer), JITCompiler::argumentRegister2);
        m_jit.move(JITCompiler::callFrameRegister, JITCompiler::argumentRegister0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(JITCompiler::returnValueRegister, JITCompiler::gprToRegisterID(result));
    }
    void callOperation(J_DFGOperation_EJI operation, GPRReg result, GPRReg arg1, Identifier* identifier)
    {
        callOperation((J_DFGOperation_EJP)operation, result, arg1, identifier);
    }
    void callOperation(J_DFGOperation_EJ operation, GPRReg result, GPRReg arg1)
    {
        ASSERT(isFlushed());

        m_jit.move(JITCompiler::gprToRegisterID(arg1), JITCompiler::argumentRegister1);
        m_jit.move(JITCompiler::callFrameRegister, JITCompiler::argumentRegister0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(JITCompiler::returnValueRegister, JITCompiler::gprToRegisterID(result));
    }
    void callOperation(J_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        ASSERT(isFlushed());

        setupStubArguments(arg1, arg2);
        m_jit.move(JITCompiler::callFrameRegister, JITCompiler::argumentRegister0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(JITCompiler::returnValueRegister, JITCompiler::gprToRegisterID(result));
    }
    void callOperation(V_DFGOperation_EJJP operation, GPRReg arg1, GPRReg arg2, void* pointer)
    {
        ASSERT(isFlushed());

        setupStubArguments(arg1, arg2);
        m_jit.move(JITCompiler::TrustedImmPtr(pointer), JITCompiler::argumentRegister3);
        m_jit.move(JITCompiler::callFrameRegister, JITCompiler::argumentRegister0);

        appendCallWithExceptionCheck(operation);
    }
    void callOperation(V_DFGOperation_EJJI operation, GPRReg arg1, GPRReg arg2, Identifier* identifier)
    {
        callOperation((V_DFGOperation_EJJP)operation, arg1, arg2, identifier);
    }
    void callOperation(V_DFGOperation_EJJJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        ASSERT(isFlushed());

        setupStubArguments(arg1, arg2, arg3);
        m_jit.move(JITCompiler::callFrameRegister, JITCompiler::argumentRegister0);

        appendCallWithExceptionCheck(operation);
    }
    void callOperation(D_DFGOperation_DD operation, FPRReg result, FPRReg arg1, FPRReg arg2)
    {
        ASSERT(isFlushed());

        setupTwoStubArgs<JITCompiler::argumentFPR0, JITCompiler::argumentFPR1>(arg1, arg2);

        m_jit.appendCall(operation);
        m_jit.moveDouble(JITCompiler::fpReturnValueRegister, JITCompiler::fprToRegisterID(result));
    }

    void appendCallWithExceptionCheck(const FunctionPtr& function)
    {
        m_jit.appendCallWithExceptionCheck(function, m_jit.graph()[m_compileIndex].exceptionInfo);
    }

#ifndef NDEBUG
    void dump(const char* label = 0);
#endif

#if DFG_CONSISTENCY_CHECK
    void checkConsistency();
#else
    void checkConsistency() {}
#endif

    // The JIT, while also provides MacroAssembler functionality.
    JITCompiler& m_jit;
    // This flag is used to distinguish speculative and non-speculative
    // code generation. This is significant when filling spilled values
    // from the RegisterFile. When spilling we attempt to store information
    // as to the type of boxed value being stored (int32, double, cell), and
    // when filling on the speculative path we will retrieve this type info
    // where available. On the non-speculative path, however, we cannot rely
    // on the spill format info, since the a value being loaded might have
    // been spilled by either the speculative or non-speculative paths (where
    // we entered the non-speculative path on an intervening bail-out), and
    // the value may have been boxed differently on the two paths.
    bool m_isSpeculative;
    // The current node being generated.
    NodeIndex m_compileIndex;
    // Virtual and physical register maps.
    Vector<GenerationInfo, 32> m_generationInfo;
    RegisterBank<GPRReg, numberOfGPRs, SpillOrder, SpillOrderNone, SpillOrderMax> m_gprs;
    RegisterBank<FPRReg, numberOfFPRs, SpillOrder, SpillOrderNone, SpillOrderMax> m_fprs;
};

// === Operand types ===
//
// IntegerOperand, DoubleOperand and JSValueOperand.
//
// These classes are used to lock the operands to a node into machine
// registers. These classes implement of pattern of locking a value
// into register at the point of construction only if it is already in
// registers, and otherwise loading it lazily at the point it is first
// used. We do so in order to attempt to avoid spilling one operand
// in order to make space available for another.

class IntegerOperand {
public:
    explicit IntegerOperand(JITCodeGenerator* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_gprOrInvalid(InvalidGPRReg)
#ifndef NDEBUG
        , m_format(DataFormatNone)
#endif
    {
        ASSERT(m_jit);
        if (jit->isFilled(index))
            gpr();
    }

    ~IntegerOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillInteger(index(), m_format);
        return m_gprOrInvalid;
    }

    DataFormat format()
    {
        gpr(); // m_format is set when m_gpr is locked.
        ASSERT(m_format == DataFormatInteger || m_format == DataFormatJSInteger);
        return m_format;
    }

    MacroAssembler::RegisterID registerID()
    {
        return JITCompiler::gprToRegisterID(gpr());
    }

private:
    JITCodeGenerator* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
    DataFormat m_format;
};

class DoubleOperand {
public:
    explicit DoubleOperand(JITCodeGenerator* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_fprOrInvalid(InvalidFPRReg)
    {
        ASSERT(m_jit);
        if (jit->isFilledDouble(index))
            fpr();
    }

    ~DoubleOperand()
    {
        ASSERT(m_fprOrInvalid != InvalidFPRReg);
        m_jit->unlock(m_fprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    FPRReg fpr()
    {
        if (m_fprOrInvalid == InvalidFPRReg)
            m_fprOrInvalid = m_jit->fillDouble(index());
        return m_fprOrInvalid;
    }

    MacroAssembler::FPRegisterID registerID()
    {
        return JITCompiler::fprToRegisterID(fpr());
    }

private:
    JITCodeGenerator* m_jit;
    NodeIndex m_index;
    FPRReg m_fprOrInvalid;
};

class JSValueOperand {
public:
    explicit JSValueOperand(JITCodeGenerator* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        if (jit->isFilled(index))
            gpr();
    }

    ~JSValueOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillJSValue(index());
        return m_gprOrInvalid;
    }

    MacroAssembler::RegisterID registerID()
    {
        return JITCompiler::gprToRegisterID(gpr());
    }

private:
    JITCodeGenerator* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
};


// === Temporaries ===
//
// These classes are used to allocate temporary registers.
// A mechanism is provided to attempt to reuse the registers
// currently allocated to child nodes whose value is consumed
// by, and not live after, this operation.

class GPRTemporary {
public:
    GPRTemporary(JITCodeGenerator*);
    GPRTemporary(JITCodeGenerator*, SpeculateIntegerOperand&);
    GPRTemporary(JITCodeGenerator*, SpeculateIntegerOperand&, SpeculateIntegerOperand&);
    GPRTemporary(JITCodeGenerator*, IntegerOperand&);
    GPRTemporary(JITCodeGenerator*, IntegerOperand&, IntegerOperand&);
    GPRTemporary(JITCodeGenerator*, SpeculateCellOperand&);
    GPRTemporary(JITCodeGenerator*, JSValueOperand&);

    ~GPRTemporary()
    {
        m_jit->unlock(gpr());
    }

    GPRReg gpr() const
    {
        ASSERT(m_gpr != InvalidGPRReg);
        return m_gpr;
    }

    MacroAssembler::RegisterID registerID()
    {
        ASSERT(m_gpr != InvalidGPRReg);
        return JITCompiler::gprToRegisterID(m_gpr);
    }

protected:
    GPRTemporary(JITCodeGenerator* jit, GPRReg lockedGPR)
        : m_jit(jit)
        , m_gpr(lockedGPR)
    {
    }

private:
    JITCodeGenerator* m_jit;
    GPRReg m_gpr;
};

class FPRTemporary {
public:
    FPRTemporary(JITCodeGenerator*);
    FPRTemporary(JITCodeGenerator*, DoubleOperand&);
    FPRTemporary(JITCodeGenerator*, DoubleOperand&, DoubleOperand&);

    ~FPRTemporary()
    {
        m_jit->unlock(fpr());
    }

    FPRReg fpr() const
    {
        ASSERT(m_fpr != InvalidFPRReg);
        return m_fpr;
    }

    MacroAssembler::FPRegisterID registerID()
    {
        ASSERT(m_fpr != InvalidFPRReg);
        return JITCompiler::fprToRegisterID(m_fpr);
    }

protected:
    FPRTemporary(JITCodeGenerator* jit, FPRReg lockedFPR)
        : m_jit(jit)
        , m_fpr(lockedFPR)
    {
    }

private:
    JITCodeGenerator* m_jit;
    FPRReg m_fpr;
};


// === Results ===
//
// These classes lock the result of a call to a C++ helper function.

class GPRResult : public GPRTemporary {
public:
    GPRResult(JITCodeGenerator* jit)
        : GPRTemporary(jit, lockedResult(jit))
    {
    }

private:
    static GPRReg lockedResult(JITCodeGenerator* jit)
    {
        jit->lock(JITCompiler::returnValueGPR);
        return JITCompiler::returnValueGPR;
    }
};

class FPRResult : public FPRTemporary {
public:
    FPRResult(JITCodeGenerator* jit)
        : FPRTemporary(jit, lockedResult(jit))
    {
    }

private:
    static FPRReg lockedResult(JITCodeGenerator* jit)
    {
        jit->lock(JITCompiler::returnValueFPR);
        return JITCompiler::returnValueFPR;
    }
};

} } // namespace JSC::DFG

#endif
#endif

