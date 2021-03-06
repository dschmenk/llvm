//===- ARMLegalizerInfo.cpp --------------------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the Machinelegalizer class for ARM.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#include "ARMLegalizerInfo.h"
#include "ARMCallLowering.h"
#include "ARMSubtarget.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/LowLevelType.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetOpcodes.h"

using namespace llvm;

static bool AEABI(const ARMSubtarget &ST) {
  return ST.isTargetAEABI() || ST.isTargetGNUAEABI() || ST.isTargetMuslAEABI();
}

ARMLegalizerInfo::ARMLegalizerInfo(const ARMSubtarget &ST) {
  using namespace TargetOpcode;

  const LLT p0 = LLT::pointer(0, 32);

  const LLT s1 = LLT::scalar(1);
  const LLT s8 = LLT::scalar(8);
  const LLT s16 = LLT::scalar(16);
  const LLT s32 = LLT::scalar(32);
  const LLT s64 = LLT::scalar(64);

  setAction({G_GLOBAL_VALUE, p0}, Legal);
  setAction({G_FRAME_INDEX, p0}, Legal);

  for (unsigned Op : {G_LOAD, G_STORE}) {
    for (auto Ty : {s1, s8, s16, s32, p0})
      setAction({Op, Ty}, Legal);
    setAction({Op, 1, p0}, Legal);
  }

  for (unsigned Op : {G_ADD, G_SUB, G_MUL, G_AND, G_OR, G_XOR}) {
    for (auto Ty : {s1, s8, s16})
      setAction({Op, Ty}, WidenScalar);
    setAction({Op, s32}, Legal);
  }

  for (unsigned Op : {G_SDIV, G_UDIV}) {
    for (auto Ty : {s8, s16})
      setAction({Op, Ty}, WidenScalar);
    if (ST.hasDivideInARMMode())
      setAction({Op, s32}, Legal);
    else
      setAction({Op, s32}, Libcall);
  }

  for (unsigned Op : {G_SREM, G_UREM}) {
    for (auto Ty : {s8, s16})
      setAction({Op, Ty}, WidenScalar);
    if (ST.hasDivideInARMMode())
      setAction({Op, s32}, Lower);
    else if (AEABI(ST))
      setAction({Op, s32}, Custom);
    else
      setAction({Op, s32}, Libcall);
  }

  for (unsigned Op : {G_SEXT, G_ZEXT}) {
    setAction({Op, s32}, Legal);
    for (auto Ty : {s1, s8, s16})
      setAction({Op, 1, Ty}, Legal);
  }

  for (unsigned Op : {G_ASHR, G_LSHR, G_SHL})
    setAction({Op, s32}, Legal);

  setAction({G_GEP, p0}, Legal);
  setAction({G_GEP, 1, s32}, Legal);

  setAction({G_SELECT, s32}, Legal);
  setAction({G_SELECT, p0}, Legal);
  setAction({G_SELECT, 1, s1}, Legal);

  setAction({G_BRCOND, s1}, Legal);

  setAction({G_CONSTANT, s32}, Legal);
  for (auto Ty : {s1, s8, s16})
    setAction({G_CONSTANT, Ty}, WidenScalar);

  setAction({G_ICMP, s1}, Legal);
  for (auto Ty : {s8, s16})
    setAction({G_ICMP, 1, Ty}, WidenScalar);
  for (auto Ty : {s32, p0})
    setAction({G_ICMP, 1, Ty}, Legal);

  if (!ST.useSoftFloat() && ST.hasVFP2()) {
    for (unsigned BinOp : {G_FADD, G_FSUB})
      for (auto Ty : {s32, s64})
        setAction({BinOp, Ty}, Legal);

    setAction({G_LOAD, s64}, Legal);
    setAction({G_STORE, s64}, Legal);

    setAction({G_FCMP, s1}, Legal);
    setAction({G_FCMP, 1, s32}, Legal);
    setAction({G_FCMP, 1, s64}, Legal);
  } else {
    for (unsigned BinOp : {G_FADD, G_FSUB})
      for (auto Ty : {s32, s64})
        setAction({BinOp, Ty}, Libcall);

    setAction({G_FCMP, s1}, Legal);
    setAction({G_FCMP, 1, s32}, Custom);
    setAction({G_FCMP, 1, s64}, Custom);

    if (AEABI(ST))
      setFCmpLibcallsAEABI();
    else
      setFCmpLibcallsGNU();
  }

  for (unsigned Op : {G_FREM, G_FPOW})
    for (auto Ty : {s32, s64})
      setAction({Op, Ty}, Libcall);

  computeTables();
}

void ARMLegalizerInfo::setFCmpLibcallsAEABI() {
  // FCMP_TRUE and FCMP_FALSE don't need libcalls, they should be
  // default-initialized.
  FCmp32Libcalls.resize(CmpInst::LAST_FCMP_PREDICATE + 1);
  FCmp32Libcalls[CmpInst::FCMP_OEQ] = {
      {RTLIB::OEQ_F32, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp32Libcalls[CmpInst::FCMP_OGE] = {
      {RTLIB::OGE_F32, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp32Libcalls[CmpInst::FCMP_OGT] = {
      {RTLIB::OGT_F32, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp32Libcalls[CmpInst::FCMP_OLE] = {
      {RTLIB::OLE_F32, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp32Libcalls[CmpInst::FCMP_OLT] = {
      {RTLIB::OLT_F32, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp32Libcalls[CmpInst::FCMP_ORD] = {{RTLIB::O_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_UGE] = {{RTLIB::OLT_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_UGT] = {{RTLIB::OLE_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_ULE] = {{RTLIB::OGT_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_ULT] = {{RTLIB::OGE_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_UNE] = {{RTLIB::UNE_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_UNO] = {
      {RTLIB::UO_F32, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp32Libcalls[CmpInst::FCMP_ONE] = {
      {RTLIB::OGT_F32, CmpInst::BAD_ICMP_PREDICATE},
      {RTLIB::OLT_F32, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp32Libcalls[CmpInst::FCMP_UEQ] = {
      {RTLIB::OEQ_F32, CmpInst::BAD_ICMP_PREDICATE},
      {RTLIB::UO_F32, CmpInst::BAD_ICMP_PREDICATE}};

  FCmp64Libcalls.resize(CmpInst::LAST_FCMP_PREDICATE + 1);
  FCmp64Libcalls[CmpInst::FCMP_OEQ] = {
      {RTLIB::OEQ_F64, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp64Libcalls[CmpInst::FCMP_OGE] = {
      {RTLIB::OGE_F64, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp64Libcalls[CmpInst::FCMP_OGT] = {
      {RTLIB::OGT_F64, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp64Libcalls[CmpInst::FCMP_OLE] = {
      {RTLIB::OLE_F64, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp64Libcalls[CmpInst::FCMP_OLT] = {
      {RTLIB::OLT_F64, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp64Libcalls[CmpInst::FCMP_ORD] = {{RTLIB::O_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_UGE] = {{RTLIB::OLT_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_UGT] = {{RTLIB::OLE_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_ULE] = {{RTLIB::OGT_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_ULT] = {{RTLIB::OGE_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_UNE] = {{RTLIB::UNE_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_UNO] = {
      {RTLIB::UO_F64, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp64Libcalls[CmpInst::FCMP_ONE] = {
      {RTLIB::OGT_F64, CmpInst::BAD_ICMP_PREDICATE},
      {RTLIB::OLT_F64, CmpInst::BAD_ICMP_PREDICATE}};
  FCmp64Libcalls[CmpInst::FCMP_UEQ] = {
      {RTLIB::OEQ_F64, CmpInst::BAD_ICMP_PREDICATE},
      {RTLIB::UO_F64, CmpInst::BAD_ICMP_PREDICATE}};
}

void ARMLegalizerInfo::setFCmpLibcallsGNU() {
  // FCMP_TRUE and FCMP_FALSE don't need libcalls, they should be
  // default-initialized.
  FCmp32Libcalls.resize(CmpInst::LAST_FCMP_PREDICATE + 1);
  FCmp32Libcalls[CmpInst::FCMP_OEQ] = {{RTLIB::OEQ_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_OGE] = {{RTLIB::OGE_F32, CmpInst::ICMP_SGE}};
  FCmp32Libcalls[CmpInst::FCMP_OGT] = {{RTLIB::OGT_F32, CmpInst::ICMP_SGT}};
  FCmp32Libcalls[CmpInst::FCMP_OLE] = {{RTLIB::OLE_F32, CmpInst::ICMP_SLE}};
  FCmp32Libcalls[CmpInst::FCMP_OLT] = {{RTLIB::OLT_F32, CmpInst::ICMP_SLT}};
  FCmp32Libcalls[CmpInst::FCMP_ORD] = {{RTLIB::O_F32, CmpInst::ICMP_EQ}};
  FCmp32Libcalls[CmpInst::FCMP_UGE] = {{RTLIB::OLT_F32, CmpInst::ICMP_SGE}};
  FCmp32Libcalls[CmpInst::FCMP_UGT] = {{RTLIB::OLE_F32, CmpInst::ICMP_SGT}};
  FCmp32Libcalls[CmpInst::FCMP_ULE] = {{RTLIB::OGT_F32, CmpInst::ICMP_SLE}};
  FCmp32Libcalls[CmpInst::FCMP_ULT] = {{RTLIB::OGE_F32, CmpInst::ICMP_SLT}};
  FCmp32Libcalls[CmpInst::FCMP_UNE] = {{RTLIB::UNE_F32, CmpInst::ICMP_NE}};
  FCmp32Libcalls[CmpInst::FCMP_UNO] = {{RTLIB::UO_F32, CmpInst::ICMP_NE}};
  FCmp32Libcalls[CmpInst::FCMP_ONE] = {{RTLIB::OGT_F32, CmpInst::ICMP_SGT},
                                       {RTLIB::OLT_F32, CmpInst::ICMP_SLT}};
  FCmp32Libcalls[CmpInst::FCMP_UEQ] = {{RTLIB::OEQ_F32, CmpInst::ICMP_EQ},
                                       {RTLIB::UO_F32, CmpInst::ICMP_NE}};

  FCmp64Libcalls.resize(CmpInst::LAST_FCMP_PREDICATE + 1);
  FCmp64Libcalls[CmpInst::FCMP_OEQ] = {{RTLIB::OEQ_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_OGE] = {{RTLIB::OGE_F64, CmpInst::ICMP_SGE}};
  FCmp64Libcalls[CmpInst::FCMP_OGT] = {{RTLIB::OGT_F64, CmpInst::ICMP_SGT}};
  FCmp64Libcalls[CmpInst::FCMP_OLE] = {{RTLIB::OLE_F64, CmpInst::ICMP_SLE}};
  FCmp64Libcalls[CmpInst::FCMP_OLT] = {{RTLIB::OLT_F64, CmpInst::ICMP_SLT}};
  FCmp64Libcalls[CmpInst::FCMP_ORD] = {{RTLIB::O_F64, CmpInst::ICMP_EQ}};
  FCmp64Libcalls[CmpInst::FCMP_UGE] = {{RTLIB::OLT_F64, CmpInst::ICMP_SGE}};
  FCmp64Libcalls[CmpInst::FCMP_UGT] = {{RTLIB::OLE_F64, CmpInst::ICMP_SGT}};
  FCmp64Libcalls[CmpInst::FCMP_ULE] = {{RTLIB::OGT_F64, CmpInst::ICMP_SLE}};
  FCmp64Libcalls[CmpInst::FCMP_ULT] = {{RTLIB::OGE_F64, CmpInst::ICMP_SLT}};
  FCmp64Libcalls[CmpInst::FCMP_UNE] = {{RTLIB::UNE_F64, CmpInst::ICMP_NE}};
  FCmp64Libcalls[CmpInst::FCMP_UNO] = {{RTLIB::UO_F64, CmpInst::ICMP_NE}};
  FCmp64Libcalls[CmpInst::FCMP_ONE] = {{RTLIB::OGT_F64, CmpInst::ICMP_SGT},
                                       {RTLIB::OLT_F64, CmpInst::ICMP_SLT}};
  FCmp64Libcalls[CmpInst::FCMP_UEQ] = {{RTLIB::OEQ_F64, CmpInst::ICMP_EQ},
                                       {RTLIB::UO_F64, CmpInst::ICMP_NE}};
}

ARMLegalizerInfo::FCmpLibcallsList
ARMLegalizerInfo::getFCmpLibcalls(CmpInst::Predicate Predicate,
                                  unsigned Size) const {
  assert(CmpInst::isFPPredicate(Predicate) && "Unsupported FCmp predicate");
  if (Size == 32)
    return FCmp32Libcalls[Predicate];
  if (Size == 64)
    return FCmp64Libcalls[Predicate];
  llvm_unreachable("Unsupported size for FCmp predicate");
}

bool ARMLegalizerInfo::legalizeCustom(MachineInstr &MI,
                                      MachineRegisterInfo &MRI,
                                      MachineIRBuilder &MIRBuilder) const {
  using namespace TargetOpcode;

  MIRBuilder.setInstr(MI);

  switch (MI.getOpcode()) {
  default:
    return false;
  case G_SREM:
  case G_UREM: {
    unsigned OriginalResult = MI.getOperand(0).getReg();
    auto Size = MRI.getType(OriginalResult).getSizeInBits();
    if (Size != 32)
      return false;

    auto Libcall =
        MI.getOpcode() == G_SREM ? RTLIB::SDIVREM_I32 : RTLIB::UDIVREM_I32;

    // Our divmod libcalls return a struct containing the quotient and the
    // remainder. We need to create a virtual register for it.
    auto &Ctx = MIRBuilder.getMF().getFunction()->getContext();
    Type *ArgTy = Type::getInt32Ty(Ctx);
    StructType *RetTy = StructType::get(Ctx, {ArgTy, ArgTy}, /* Packed */ true);
    auto RetVal = MRI.createGenericVirtualRegister(
        getLLTForType(*RetTy, MIRBuilder.getMF().getDataLayout()));

    auto Status = createLibcall(MIRBuilder, Libcall, {RetVal, RetTy},
                                {{MI.getOperand(1).getReg(), ArgTy},
                                 {MI.getOperand(2).getReg(), ArgTy}});
    if (Status != LegalizerHelper::Legalized)
      return false;

    // The remainder is the second result of divmod. Split the return value into
    // a new, unused register for the quotient and the destination of the
    // original instruction for the remainder.
    MIRBuilder.buildUnmerge(
        {MRI.createGenericVirtualRegister(LLT::scalar(32)), OriginalResult},
        RetVal);
    break;
  }
  case G_FCMP: {
    assert(MRI.getType(MI.getOperand(2).getReg()) ==
               MRI.getType(MI.getOperand(3).getReg()) &&
           "Mismatched operands for G_FCMP");
    auto OpSize = MRI.getType(MI.getOperand(2).getReg()).getSizeInBits();

    auto OriginalResult = MI.getOperand(0).getReg();
    auto Predicate =
        static_cast<CmpInst::Predicate>(MI.getOperand(1).getPredicate());
    auto Libcalls = getFCmpLibcalls(Predicate, OpSize);

    if (Libcalls.empty()) {
      assert((Predicate == CmpInst::FCMP_TRUE ||
              Predicate == CmpInst::FCMP_FALSE) &&
             "Predicate needs libcalls, but none specified");
      MIRBuilder.buildConstant(OriginalResult,
                               Predicate == CmpInst::FCMP_TRUE ? 1 : 0);
      MI.eraseFromParent();
      return true;
    }

    auto &Ctx = MIRBuilder.getMF().getFunction()->getContext();
    assert((OpSize == 32 || OpSize == 64) && "Unsupported operand size");
    auto *ArgTy = OpSize == 32 ? Type::getFloatTy(Ctx) : Type::getDoubleTy(Ctx);
    auto *RetTy = Type::getInt32Ty(Ctx);

    SmallVector<unsigned, 2> Results;
    for (auto Libcall : Libcalls) {
      auto LibcallResult = MRI.createGenericVirtualRegister(LLT::scalar(32));
      auto Status =
          createLibcall(MIRBuilder, Libcall.LibcallID, {LibcallResult, RetTy},
                        {{MI.getOperand(2).getReg(), ArgTy},
                         {MI.getOperand(3).getReg(), ArgTy}});

      if (Status != LegalizerHelper::Legalized)
        return false;

      auto ProcessedResult =
          Libcalls.size() == 1
              ? OriginalResult
              : MRI.createGenericVirtualRegister(MRI.getType(OriginalResult));

      // We have a result, but we need to transform it into a proper 1-bit 0 or
      // 1, taking into account the different peculiarities of the values
      // returned by the comparison functions.
      CmpInst::Predicate ResultPred = Libcall.Predicate;
      if (ResultPred == CmpInst::BAD_ICMP_PREDICATE) {
        // We have a nice 0 or 1, and we just need to truncate it back to 1 bit
        // to keep the types consistent.
        MIRBuilder.buildTrunc(ProcessedResult, LibcallResult);
      } else {
        // We need to compare against 0.
        assert(CmpInst::isIntPredicate(ResultPred) && "Unsupported predicate");
        auto Zero = MRI.createGenericVirtualRegister(LLT::scalar(32));
        MIRBuilder.buildConstant(Zero, 0);
        MIRBuilder.buildICmp(ResultPred, ProcessedResult, LibcallResult, Zero);
      }
      Results.push_back(ProcessedResult);
    }

    if (Results.size() != 1) {
      assert(Results.size() == 2 && "Unexpected number of results");
      MIRBuilder.buildOr(OriginalResult, Results[0], Results[1]);
    }
    break;
  }
  }

  MI.eraseFromParent();
  return true;
}
