/*******************************************************************************
 * Copyright IBM Corp. and others 2000
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef OMR_X86_INSTRUCTION_INCL
#define OMR_X86_INSTRUCTION_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_INSTRUCTION_CONNECTOR
#define OMR_INSTRUCTION_CONNECTOR
namespace OMR { namespace X86 { class Instruction; } }
namespace OMR { typedef OMR::X86::Instruction InstructionConnector; }
#else
#error OMR::X86::Instruction expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/codegen/OMRInstruction.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/InstOpCode.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/InstOpCode.hpp"

namespace TR { class X86ImmInstruction;   }
namespace TR { class X86LabelInstruction; }
namespace TR { class X86RegInstruction;   }
namespace TR { class CodeGenerator; }
namespace TR { class Instruction; }
namespace TR { class MemoryReference; }
namespace TR { class Node; }
namespace TR { class Register; }
namespace TR { class RegisterDependencyConditions; }
struct TR_VFPState;

namespace OMR
{

namespace X86
{
class EnlargementResult
   {
   int32_t _patchGrowth; // the minimum guaratneed size of increase - we know we will grow this much
   int32_t _encodingGrowth; // the maximum size of increase - we have to accomodate growth of up to this much
   public:
   EnlargementResult(int32_t patchGrowth, int32_t encodingGrowth)
      : _patchGrowth(patchGrowth), _encodingGrowth(encodingGrowth) { }
   EnlargementResult(const EnlargementResult &other)
      : _patchGrowth(other._patchGrowth), _encodingGrowth(other._encodingGrowth) { }
   int32_t getPatchGrowth() { return _patchGrowth; }
   int32_t getEncodingGrowth() { return _encodingGrowth; }
   };

class OMR_EXTENSIBLE Instruction : public OMR::Instruction
   {
   private:
   uint8_t _rexRepeatCount;
   OMR::X86::Encoding _encodingMethod;

   protected:
#if defined(TR_TARGET_64BIT)
   void setRexRepeatCount(uint8_t count) { _rexRepeatCount = count; }
#else
   void setRexRepeatCount(uint8_t count) { TR_ASSERT(0, "Forcing a rex repetition does not make sense in IA32 mode!"); }
#endif

   protected:

   Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node, OMR::X86::Encoding encoding = Default);
   Instruction(TR::CodeGenerator *cg, TR::Instruction *precedingInstruction, TR::InstOpCode::Mnemonic op, TR::Node *node = 0, OMR::X86::Encoding encoding = Default);
   void initialize(TR::CodeGenerator *cg = NULL, TR::RegisterDependencyConditions *cond = NULL, TR::InstOpCode::Mnemonic op = TR::InstOpCode::bad, bool flag = false);

   public:

   virtual const char *description() { return "X86"; }
   virtual Kind getKind() { return IsNotExtended; }

   OMR::X86::Encoding getEncodingMethod() { return _encodingMethod; }
   void setEncodingMethod(OMR::X86::Encoding method) { _encodingMethod = method; }

   virtual bool isBranchOp() {return _opcode.isBranchOp() != 0;}
   virtual bool isLabel();
   virtual bool isRegRegMove();
   virtual bool isPatchBarrier(TR::CodeGenerator *cg);

   TR::RegisterDependencyConditions *getDependencyConditions() {return _conditions;}
   TR::RegisterDependencyConditions *setDependencyConditions(TR::RegisterDependencyConditions *cond) { return (_conditions = cond); }

   virtual void assignRegisters(TR_RegisterKinds kindsToBeAssigned);
   virtual bool refsRegister(TR::Register *reg);
   virtual bool defsRegister(TR::Register *reg);
   virtual bool usesRegister(TR::Register *reg);
   virtual bool dependencyRefsRegister(TR::Register *reg);

   virtual uint8_t *generateBinaryEncoding();
   virtual int32_t  estimateBinaryLength(int32_t currentEstimate);
   virtual uint8_t* generateOperand(uint8_t* cursor) { return cursor; }
   virtual bool     needsRepPrefix();
   virtual bool     needsLockPrefix();
   virtual EnlargementResult enlarge(int32_t requestedEnlargementSize, int32_t maxEnlargementSize, bool allowPartialEnlargement) { return EnlargementResult(0, 0); }

   virtual TR::X86RegInstruction *getX86RegInstruction() { return NULL; }

   virtual TR::X86LabelInstruction *getX86LabelInstruction() { return NULL; }

#if defined(DEBUG) || defined(PROD_WITH_ASSUMES)
   // The following safe virtual downcast method is used under debug only
   // for assertion checking.
   //
   virtual TR::X86ImmInstruction *getX86ImmInstruction() { return NULL; }

   virtual uint32_t getNumOperandReferencedGPRegisters() { return 0; }
   virtual uint32_t getNumOperandReferencedFPRegisters() { return 0; }
   uint32_t totalReferencedGPRegisters(TR::CodeGenerator *);
   uint32_t totalReferencedFPRegisters(TR::CodeGenerator *);
#endif

   // AMD64-specific REX prefix calculations
   // Note the polymorphism
   //

   // Number of unnecessary REX bytes for instruction expansion and/or padding
   uint8_t rexRepeatCount();
   // Only repeated REX bytes for expansion/padding are generated
   uint8_t *generateRepeatedRexPrefix(uint8_t *cursor);

#if defined(TR_TARGET_64BIT)
   uint8_t operandSizeRexBits();
   // Each subclass should override this as necessary
   virtual uint8_t rexBits();
#else
   uint8_t rexBits() { return 0; }
#endif

   // Adjust the VFP state to reflect the execution of this instruction.
   //
   virtual void adjustVFPState(TR_VFPState *state, TR::CodeGenerator *cg);

   virtual TR::Register *getTargetRegister() { return NULL; }
   virtual TR::Register *getSourceRegister() { return NULL; }
   virtual TR::Register *getSource2ndRegister() { return NULL; }
   virtual TR::Register *getMaskRegister() { return NULL; }
   virtual TR::MemoryReference *getMemoryReference() { return NULL; }
   virtual bool hasZeroMask() { return false; }

   int32_t getMaxPatchableInstructionLength() { return 10; }

   protected:

   // Facility for subclasses to compute effect of a call on the VFP state.
   // If _opcode->isCallOp(), adds vfpAdjustmentForCall; else, calls TR::Instruction ::adjustVFPState.
   //
   void adjustVFPStateForCall(TR_VFPState *state, int32_t vfpAdjustmentForCall, TR::CodeGenerator *cg);
   void clobberRegsForRematerialisation();

   private:

   TR::RegisterDependencyConditions *_conditions;
   void assumeValidInstruction();

   public:

   struct ModRM
      {
      uint8_t rm : 3;
      uint8_t reg : 3;
      uint8_t mod : 2;
      inline ModRM() {}
      inline ModRM(uint8_t opcode)
         {
         rm = 0;
         reg = opcode;
         mod = 0x3;
         }
      inline ModRM(const ModRM& other)
         {
         rm = other.rm;
         reg = other.reg;
         mod = other.mod;
         }
      inline operator uint8_t() const
         {
         return *((uint8_t*)this);
         }
      inline uint8_t Reg(uint8_t R = 0) const
         {
         return (R << 3) | (0x7 & reg);
         }
      inline uint8_t RM(uint8_t B = 0) const
         {
         TR_ASSERT(mod == 0x3, "ModRM is not in register mode");
         return (B << 3) | (0x7 & rm);
         }
      inline ModRM* setMod(uint8_t mod = 0x03) // 0b11
         {
         this->mod = mod;
         return this;
         }
      inline ModRM* setBase()
         {
         return setMod(0x00); // 0b00
         }
      inline ModRM* setBaseDisp8()
         {
         return setMod(0x01); // 0b01
         }
      inline ModRM* setBaseDisp32()
         {
         return setMod(0x02); // 0b10
         }
      inline ModRM* setIndexOnlyDisp32()
         {
         rm = 0x05; // 0b101
         return setMod(0x00); // 0b00
         }
      inline ModRM* setHasSIB()
         {
         rm = 0x04; // 0b100
         return this;
         }
      };
   struct SIB
      {
      uint8_t base : 3;
      uint8_t index : 3;
      uint8_t scale : 2;
      inline operator uint8_t() const
         {
         return *((uint8_t*)this);
         }
      inline SIB* setScale(uint8_t scale = 0)
         {
         this->scale = scale;
         return this;
         }
      inline SIB* setNoIndex()
         {
         index = 0x04; // 0b100
         return this;
         }
      inline SIB* setIndexDisp32()
         {
         base = 0x05; // 0b101
         return this;
         }
      };
   struct REX
      {
      uint8_t B : 1;
      uint8_t X : 1;
      uint8_t R : 1;
      uint8_t W : 1;
      uint8_t _padding : 4;
      inline REX(uint8_t val = 0)
         {
         *((uint8_t*)this) = val;
         _padding = 0x4;
         }
      inline uint8_t value() const
         {
         return 0x0f & *((uint8_t*)this);
         }
      };
   template<size_t VEX_SIZE>
   struct VEX
      {
      VEX() {TR_ASSERT_FATAL(false, "INVALID VEX PREFIX");}
      };

   struct EVEX
      {
      // 0x62 P0 P1 P2
      // Byte 0: 0x62
      uint8_t escape;
      // Byte 1 : P0
      uint8_t mm : 2;   // P0[0] : m0, m1
      uint8_t zero : 2; // P0[2] : 0b00
      uint8_t r : 1;    // P0[4] : R'
      uint8_t B : 1;    // P0[5] : B
      uint8_t X : 1;    // P0[6] : X
      uint8_t R : 1;    // P0[7] : R
      // Byte 2 : P1
      uint8_t p : 2;    // P1[0] : p1, p0
      uint8_t one : 1;  // P1[2] : 0b1
      uint8_t v : 4;    // P1[3] : v0, v1, v2, v3
      uint8_t W : 1;    // P1[7] : W

      // Byte 3 : P2
      uint8_t a : 3;    // P2[0] : a0, a1, a2 -- write mask {k1-k7}
      uint8_t V : 1;    // P2[3] : V'
      uint8_t b : 1;    // P2[4] : b
      uint8_t L : 2;    // P2[5] : L, L'
      uint8_t Z : 1;    // P2[7] : z
      // Byte 4: opcode
      uint8_t opcode;

      inline EVEX() {}

      inline EVEX(const REX& rex, uint8_t ModRMOpCode)
         {
         escape = '\x62';
         // reserved bits
         one = 1;
         zero = 0;
         Z = 0;
         b = 0;

         R = ~rex.R;
         X = ~rex.X;
         B = ~rex.B;

         ModRM modrm(ModRMOpCode);
         r = ~(rex.R & modrm.reg);

         W = rex.W;
         v = 0xf; //0b1111
         V = v >> 3;
         a = 0;
         }

      inline uint8_t Reg(const ModRM modrm) const
         {
         return modrm.Reg(~R);
         }
      inline uint8_t RM(const ModRM modrm) const
         {
         return modrm.RM(~B);
         }
      };
   };

   template<>
   struct Instruction::VEX<3>
      {
      // Byte 0: C4
      uint8_t escape;
      // Byte 1
      uint8_t m : 5;
      uint8_t B : 1;
      uint8_t X : 1;
      uint8_t R : 1;
      // Byte 2
      uint8_t p : 2;
      uint8_t L : 1;
      uint8_t v : 4;
      uint8_t W : 1;
      // Byte 3: opcode
      uint8_t opcode;

      inline VEX() {}
      inline VEX(const REX& rex)
         {
         escape = '\xC4';
         R = ~rex.R;
         X = ~rex.X;
         B = ~rex.B;
         W = rex.W;
         v = 0xf; //0b1111
         }
      inline bool CanBeShortened() const
         {
         return X && B && !W && (m == 1);
         }
      inline uint8_t Reg(const ModRM modrm) const
         {
         return modrm.Reg(~R);
         }
      inline uint8_t RM(const ModRM modrm) const
         {
         return modrm.RM(~B);
         }
      };
   template<>
   struct Instruction::VEX<2>
      {
      // Byte 0: C5
      uint8_t escape;
      // Byte 1
      uint8_t p : 2;
      uint8_t L : 1;
      uint8_t v : 4;
      uint8_t R : 1;
      // Byte 2: opcode
      uint8_t opcode;

      inline VEX() {}
      inline VEX(const VEX<3>& other)
         {
         escape = '\xC5';
         p = other.p;
         L = other.L;
         v = other.v;
         R = other.R;
         opcode = other.opcode;
         }
      inline uint8_t Reg(const ModRM modrm) const
         {
         return modrm.Reg(~R);
         }
      inline uint8_t RM(const ModRM modrm) const
         {
         return modrm.RM();
         }
      };
}

}

#endif
