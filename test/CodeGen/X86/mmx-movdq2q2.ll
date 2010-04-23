; RUN: llc < %s -mtriple=x86_64-apple-darwin -mattr=+mmx,+sse2 | grep movdq2q | count 2
define void @t2(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to <4 x i16>
        %tmp2 = bitcast double %b to <4 x i16>
        %tmp3 = add <4 x i16> %tmp1, %tmp2
        store <4 x i16> %tmp3, <4 x i16>* null
        ret void
}
