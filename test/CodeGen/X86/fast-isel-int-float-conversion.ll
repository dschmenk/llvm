; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=x86_64-unknown-unknown -mcpu=generic -mattr=+sse2 -fast-isel --fast-isel-abort=1 < %s | FileCheck %s --check-prefix=ALL --check-prefix=SSE2
; RUN: llc -mtriple=x86_64-unknown-unknown -mcpu=generic -mattr=+avx -fast-isel --fast-isel-abort=1 < %s | FileCheck %s --check-prefix=ALL --check-prefix=AVX


define double @int_to_double_rr(i32 %a) {
; SSE2-LABEL: int_to_double_rr:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    cvtsi2sdl %edi, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: int_to_double_rr:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vcvtsi2sdl %edi, %xmm0, %xmm0
; AVX-NEXT:    retq
entry:
  %0 = sitofp i32 %a to double
  ret double %0
}

define double @int_to_double_rm(i32* %a) {
; SSE2-LABEL: int_to_double_rm:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    cvtsi2sdl (%rdi), %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: int_to_double_rm:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vcvtsi2sdl (%rdi), %xmm0, %xmm0
; AVX-NEXT:    retq
entry:
  %0 = load i32, i32* %a
  %1 = sitofp i32 %0 to double
  ret double %1
}

define float @int_to_float_rr(i32 %a) {
; SSE2-LABEL: int_to_float_rr:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    cvtsi2ssl %edi, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: int_to_float_rr:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vcvtsi2ssl %edi, %xmm0, %xmm0
; AVX-NEXT:    retq
entry:
  %0 = sitofp i32 %a to float
  ret float %0
}

define float @int_to_float_rm(i32* %a) {
; SSE2-LABEL: int_to_float_rm:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    cvtsi2ssl (%rdi), %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: int_to_float_rm:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vcvtsi2ssl (%rdi), %xmm0, %xmm0
; AVX-NEXT:    retq
entry:
  %0 = load i32, i32* %a
  %1 = sitofp i32 %0 to float
  ret float %1
}
