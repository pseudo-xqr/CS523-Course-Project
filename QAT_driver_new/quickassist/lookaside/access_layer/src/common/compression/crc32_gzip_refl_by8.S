;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2019 Intel Corporation All rights reserved.
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;    * Redistributions of source code must retain the above copyright
;      notice, this list of conditions and the following disclaimer.
;    * Redistributions in binary form must reproduce the above copyright
;      notice, this list of conditions and the following disclaimer in
;      the documentation and/or other materials provided with the
;      distribution.
;    * Neither the name of Intel Corporation nor the names of its
;      contributors may be used to endorse or promote products derived
;      from this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;       Function API:
;       UINT32 crc32_gzip_refl_by8(
;               UINT32 init_crc, //initial CRC value, 32 bits
;               const unsigned char *buf, //buffer pointer to calculate CRC on
;               UINT64 len //buffer length in bytes (64-bit data)
;       );
;
;       Authors:
;               Erdinc Ozturk
;               Vinodh Gopal
;               James Guilford
;
;       Reference paper titled "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
;       URL: http://download.intel.com/design/intarch/papers/323102.pdf
;
;
;       sample yasm command line:
;       yasm -f x64 -f elf64 -X gnu -g dwarf2 crc32_gzip_refl_by8
;
;       As explained here:
;       http://docs.oracle.com/javase/7/docs/api/java/util/zip/package-summary.html
;       CRC-32 checksum is described in RFC 1952
;       Implementing RFC 1952 CRC:
;       http://www.ietf.org/rfc/rfc1952.txt

%include "reg_sizes.asm"

%define	fetch_dist	1024

[bits 64]
default rel

section .text


%ifidn __OUTPUT_FORMAT__, win64
        %xdefine        arg1 rcx
        %xdefine        arg2 rdx
        %xdefine        arg3 r8

        %xdefine        arg1_low32 ecx
%else
        %xdefine        arg1 rdi
        %xdefine        arg2 rsi
        %xdefine        arg3 rdx

        %xdefine        arg1_low32 edi
%endif

%define TMP 16*0
%ifidn __OUTPUT_FORMAT__, win64
        %define XMM_SAVE 16*2
        %define VARIABLE_OFFSET 16*10+8
%else
        %define VARIABLE_OFFSET 16*2+8
%endif

align 16
global  crc32_gzip_refl_by8:ISAL_SYM_TYPE_FUNCTION
crc32_gzip_refl_by8:

        ; unsigned long c = crc ^ 0xffffffffL;
        not     arg1_low32	;


        sub     rsp, VARIABLE_OFFSET
%ifidn __OUTPUT_FORMAT__, win64
        ; push the xmm registers into the stack to maintain
        movdqa  [rsp + XMM_SAVE + 16*0], xmm6
        movdqa  [rsp + XMM_SAVE + 16*1], xmm7
        movdqa  [rsp + XMM_SAVE + 16*2], xmm8
        movdqa  [rsp + XMM_SAVE + 16*3], xmm9
        movdqa  [rsp + XMM_SAVE + 16*4], xmm10
        movdqa  [rsp + XMM_SAVE + 16*5], xmm11
        movdqa  [rsp + XMM_SAVE + 16*6], xmm12
        movdqa  [rsp + XMM_SAVE + 16*7], xmm13
%endif

        ; check if smaller than 256B
        cmp     arg3, 256

        ; for sizes less than 256, we can't fold 128B at a time...
        jl      _less_than_256


        ; load the initial crc value
        movd    xmm10, arg1_low32      ; initial crc

        ; receive the initial 64B data, xor the initial crc value
        movdqu  xmm0, [arg2+16*0]
        movdqu  xmm1, [arg2+16*1]
        movdqu  xmm2, [arg2+16*2]
        movdqu  xmm3, [arg2+16*3]
        movdqu  xmm4, [arg2+16*4]
        movdqu  xmm5, [arg2+16*5]
        movdqu  xmm6, [arg2+16*6]
        movdqu  xmm7, [arg2+16*7]

        ; XOR the initial_crc value
        pxor    xmm0, xmm10
	movdqa	xmm10, [rk3]	;xmm10 has rk3 and rk4
                                        ;imm value of pclmulqdq instruction will determine which constant to use
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ; we subtract 256 instead of 128 to save one instruction from the loop
        sub     arg3, 256

        ; at this section of the code, there is 128*x+y (0<=y<128) bytes of buffer. The _fold_128_B_loop
        ; loop will fold 128B at a time until we have 128+y Bytes of buffer


        ; fold 128B at a time. This section of the code folds 8 xmm registers in parallel
_fold_128_B_loop:

        ; update the buffer pointer
        add     arg2, 128

	prefetchnta [arg2+fetch_dist+0]
        movdqu  xmm9, [arg2+16*0]
        movdqu  xmm12, [arg2+16*1]
        movdqa  xmm8, xmm0
        movdqa  xmm13, xmm1
        pclmulqdq       xmm0, xmm10, 0x10
        pclmulqdq       xmm8, xmm10 , 0x1
        pclmulqdq       xmm1, xmm10, 0x10
        pclmulqdq       xmm13, xmm10 , 0x1
        pxor    xmm0, xmm9
        xorps   xmm0, xmm8
        pxor    xmm1, xmm12
        xorps   xmm1, xmm13

	prefetchnta [arg2+fetch_dist+32]
        movdqu  xmm9, [arg2+16*2]
        movdqu  xmm12, [arg2+16*3]
        movdqa  xmm8, xmm2
        movdqa  xmm13, xmm3
        pclmulqdq       xmm2, xmm10, 0x10
        pclmulqdq       xmm8, xmm10 , 0x1
        pclmulqdq       xmm3, xmm10, 0x10
        pclmulqdq       xmm13, xmm10 , 0x1
        pxor    xmm2, xmm9
        xorps   xmm2, xmm8
        pxor    xmm3, xmm12
        xorps   xmm3, xmm13

	prefetchnta [arg2+fetch_dist+64]
        movdqu  xmm9, [arg2+16*4]
        movdqu  xmm12, [arg2+16*5]
        movdqa  xmm8, xmm4
        movdqa  xmm13, xmm5
        pclmulqdq       xmm4, xmm10, 0x10
        pclmulqdq       xmm8, xmm10 , 0x1
        pclmulqdq       xmm5, xmm10, 0x10
        pclmulqdq       xmm13, xmm10 , 0x1
        pxor    xmm4, xmm9
        xorps   xmm4, xmm8
        pxor    xmm5, xmm12
        xorps   xmm5, xmm13

	prefetchnta [arg2+fetch_dist+96]
        movdqu  xmm9, [arg2+16*6]
        movdqu  xmm12, [arg2+16*7]
        movdqa  xmm8, xmm6
        movdqa  xmm13, xmm7
        pclmulqdq       xmm6, xmm10, 0x10
        pclmulqdq       xmm8, xmm10 , 0x1
        pclmulqdq       xmm7, xmm10, 0x10
        pclmulqdq       xmm13, xmm10 , 0x1
        pxor    xmm6, xmm9
        xorps   xmm6, xmm8
        pxor    xmm7, xmm12
        xorps   xmm7, xmm13

        sub     arg3, 128

        ; check if there is another 128B in the buffer to be able to fold
        jge     _fold_128_B_loop
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


        add     arg2, 128
        ; at this point, the buffer pointer is pointing at the last y Bytes of the buffer, where 0 <= y < 128
        ; the 128B of folded data is in 8 of the xmm registers: xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7


        ; fold the 8 xmm registers to 1 xmm register with different constants

        movdqa  xmm10, [rk9]
        movdqa  xmm8, xmm0
        pclmulqdq       xmm0, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        xorps   xmm7, xmm0

        movdqa  xmm10, [rk11]
        movdqa  xmm8, xmm1
        pclmulqdq       xmm1, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        xorps   xmm7, xmm1

        movdqa  xmm10, [rk13]
        movdqa  xmm8, xmm2
        pclmulqdq       xmm2, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        pxor    xmm7, xmm2

        movdqa  xmm10, [rk15]
        movdqa  xmm8, xmm3
        pclmulqdq       xmm3, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        xorps   xmm7, xmm3

        movdqa  xmm10, [rk17]
        movdqa  xmm8, xmm4
        pclmulqdq       xmm4, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        pxor    xmm7, xmm4

        movdqa  xmm10, [rk19]
        movdqa  xmm8, xmm5
        pclmulqdq       xmm5, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        xorps   xmm7, xmm5

        movdqa  xmm10, [rk1]
        movdqa  xmm8, xmm6
        pclmulqdq       xmm6, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        pxor    xmm7, xmm6


        ; instead of 128, we add 128-16 to the loop counter to save 1 instruction from the loop
        ; instead of a cmp instruction, we use the negative flag with the jl instruction
        add     arg3, 128-16
        jl      _final_reduction_for_128

        ; now we have 16+y bytes left to reduce. 16 Bytes is in register xmm7 and the rest is in memory
        ; we can fold 16 bytes at a time if y>=16
        ; continue folding 16B at a time

_16B_reduction_loop:
        movdqa  xmm8, xmm7
        pclmulqdq       xmm7, xmm10, 0x1
        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        movdqu  xmm0, [arg2]
        pxor    xmm7, xmm0
        add     arg2, 16
        sub     arg3, 16
        ; instead of a cmp instruction, we utilize the flags with the jge instruction
        ; equivalent of: cmp arg3, 16-16
        ; check if there is any more 16B in the buffer to be able to fold
        jge     _16B_reduction_loop

        ;now we have 16+z bytes left to reduce, where 0<= z < 16.
        ;first, we reduce the data in the xmm7 register


_final_reduction_for_128:
        add arg3, 16
        je _128_done

; here we are getting data that is less than 16 bytes.
        ; since we know that there was data before the pointer, we can offset the input pointer before the actual point, to receive exactly 16 bytes.
        ; after that the registers need to be adjusted.
_get_last_two_xmms:


        movdqa xmm2, xmm7
        movdqu xmm1, [arg2 - 16 + arg3]

        ; get rid of the extra data that was loaded before
        ; load the shift constant
        lea     rax, [pshufb_shf_table]
        add     rax, arg3
        movdqu  xmm0, [rax]


        pshufb  xmm7, xmm0
        pxor    xmm0, [mask3]
        pshufb  xmm2, xmm0

        pblendvb        xmm2, xmm1, xmm0     ;Last operand is fixed xmm0
        ;;;;;;;;;;
        movdqa  xmm8, xmm7
        pclmulqdq       xmm7, xmm10, 0x1

        pclmulqdq       xmm8, xmm10, 0x10
        pxor    xmm7, xmm8
        pxor    xmm7, xmm2

_128_done:
        ; compute crc of a 128-bit value
        movdqa  xmm10, [rk5]
        movdqa  xmm0, xmm7

        ;64b fold
        pclmulqdq       xmm7, xmm10, 0
        psrldq  xmm0, 8
        pxor    xmm7, xmm0

        ;32b fold
        movdqa  xmm0, xmm7
        pslldq  xmm7, 4
        pclmulqdq       xmm7, xmm10, 0x10

        pxor    xmm7, xmm0


        ;barrett reduction
_barrett:
        pand    xmm7, [mask2]
        movdqa  xmm1, xmm7
        movdqa  xmm2, xmm7
        movdqa  xmm10, [rk7]

        pclmulqdq       xmm7, xmm10, 0
        pxor    xmm7, xmm2
        pand    xmm7, [mask]
        movdqa  xmm2, xmm7
        pclmulqdq       xmm7, xmm10, 0x10
        pxor    xmm7, xmm2
        pxor    xmm7, xmm1
        pextrd  eax, xmm7, 2

_cleanup:
        ; return c ^ 0xffffffffL;
        not     eax


%ifidn __OUTPUT_FORMAT__, win64
        movdqa  xmm6, [rsp + XMM_SAVE + 16*0]
        movdqa  xmm7, [rsp + XMM_SAVE + 16*1]
        movdqa  xmm8, [rsp + XMM_SAVE + 16*2]
        movdqa  xmm9, [rsp + XMM_SAVE + 16*3]
        movdqa  xmm10, [rsp + XMM_SAVE + 16*4]
        movdqa  xmm11, [rsp + XMM_SAVE + 16*5]
        movdqa  xmm12, [rsp + XMM_SAVE + 16*6]
        movdqa  xmm13, [rsp + XMM_SAVE + 16*7]
%endif
        add     rsp, VARIABLE_OFFSET
        ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 16
_less_than_256:

        ; check if there is enough buffer to be able to fold 16B at a time
        cmp     arg3, 32
        jl      _less_than_32

        ; if there is, load the constants
        movdqa  xmm10, [rk1]    ; rk1 and rk2 in xmm10

        movd    xmm0, arg1_low32       ; get the initial crc value
        movdqu  xmm7, [arg2]            ; load the plaintext
        pxor    xmm7, xmm0

        ; update the buffer pointer
        add     arg2, 16

        ; update the counter. subtract 32 instead of 16 to save one instruction from the loop
        sub     arg3, 32

        jmp     _16B_reduction_loop


align 16
_less_than_32:
        ; mov initial crc to the return value. this is necessary for zero-length buffers.
        mov     eax, arg1_low32
        test    arg3, arg3
        je      _cleanup

        movd    xmm0, arg1_low32        ; get the initial crc value

        cmp     arg3, 16
        je      _exact_16_left
        jl      _less_than_16_left

        movdqu  xmm7, [arg2]            ; load the plaintext
        pxor    xmm7, xmm0              ; xor the initial crc value
        add     arg2, 16
        sub     arg3, 16
        movdqa  xmm10, [rk1]    ; rk1 and rk2 in xmm10
        jmp     _get_last_two_xmms


align 16
_less_than_16_left:
        ; use stack space to load data less than 16 bytes, zero-out the 16B in memory first.

        pxor    xmm1, xmm1
        mov     r11, rsp
        movdqa  [r11], xmm1

        cmp     arg3, 4
        jl      _only_less_than_4

        ;       backup the counter value
        mov     r9, arg3
        cmp     arg3, 8
        jl      _less_than_8_left

        ; load 8 Bytes
        mov     rax, [arg2]
        mov     [r11], rax
        add     r11, 8
        sub     arg3, 8
        add     arg2, 8
_less_than_8_left:

        cmp     arg3, 4
        jl      _less_than_4_left

        ; load 4 Bytes
        mov     eax, [arg2]
        mov     [r11], eax
        add     r11, 4
        sub     arg3, 4
        add     arg2, 4
_less_than_4_left:

        cmp     arg3, 2
        jl      _less_than_2_left

        ; load 2 Bytes
        mov     ax, [arg2]
        mov     [r11], ax
        add     r11, 2
        sub     arg3, 2
        add     arg2, 2
_less_than_2_left:
        cmp     arg3, 1
        jl      _zero_left

        ; load 1 Byte
        mov     al, [arg2]
        mov     [r11], al

_zero_left:
        movdqa  xmm7, [rsp]
        pxor    xmm7, xmm0      ; xor the initial crc value

        lea rax,[pshufb_shf_table]
        movdqu  xmm0, [rax + r9]
        pshufb  xmm7,xmm0



        jmp     _128_done

align 16
_exact_16_left:
        movdqu  xmm7, [arg2]
        pxor    xmm7, xmm0      ; xor the initial crc value

        jmp     _128_done

_only_less_than_4:
        cmp     arg3, 3
        jl      _only_less_than_3

        ; load 3 Bytes
        mov     al, [arg2]
        mov     [r11], al

        mov     al, [arg2+1]
        mov     [r11+1], al

        mov     al, [arg2+2]
        mov     [r11+2], al

        movdqa  xmm7, [rsp]
        pxor    xmm7, xmm0      ; xor the initial crc value

        pslldq  xmm7, 5

        jmp     _barrett
_only_less_than_3:
        cmp     arg3, 2
        jl      _only_less_than_2

        ; load 2 Bytes
        mov     al, [arg2]
        mov     [r11], al

        mov     al, [arg2+1]
        mov     [r11+1], al

        movdqa  xmm7, [rsp]
        pxor    xmm7, xmm0      ; xor the initial crc value

        pslldq  xmm7, 6

        jmp     _barrett
_only_less_than_2:

        ; load 1 Byte
        mov     al, [arg2]
        mov     [r11], al

        movdqa  xmm7, [rsp]
        pxor    xmm7, xmm0      ; xor the initial crc value

        pslldq  xmm7, 7

        jmp     _barrett

section .data

; precomputed constants
align 16
rk1 :
DQ 0x00000000ccaa009e
rk2 :
DQ 0x00000001751997d0
rk3 :
DQ 0x000000014a7fe880
rk4 :
DQ 0x00000001e88ef372
rk5 :
DQ 0x00000000ccaa009e
rk6 :
DQ 0x0000000163cd6124
rk7 :
DQ 0x00000001f7011640
rk8 :
DQ 0x00000001db710640
rk9 :
DQ 0x00000001d7cfc6ac
rk10 :
DQ 0x00000001ea89367e
rk11 :
DQ 0x000000018cb44e58
rk12 :
DQ 0x00000000df068dc2
rk13 :
DQ 0x00000000ae0b5394
rk14 :
DQ 0x00000001c7569e54
rk15 :
DQ 0x00000001c6e41596
rk16 :
DQ 0x0000000154442bd4
rk17 :
DQ 0x0000000174359406
rk18 :
DQ 0x000000003db1ecdc
rk19 :
DQ 0x000000015a546366
rk20 :
DQ 0x00000000f1da05aa

mask:
dq     0xFFFFFFFFFFFFFFFF, 0x0000000000000000
mask2:
dq     0xFFFFFFFF00000000, 0xFFFFFFFFFFFFFFFF
mask3:
dq     0x8080808080808080, 0x8080808080808080

pshufb_shf_table:
; use these values for shift constants for the pshufb instruction
; different alignments result in values as shown:
;       dq 0x8887868584838281, 0x008f8e8d8c8b8a89 ; shl 15 (16-1) / shr1
;       dq 0x8988878685848382, 0x01008f8e8d8c8b8a ; shl 14 (16-3) / shr2
;       dq 0x8a89888786858483, 0x0201008f8e8d8c8b ; shl 13 (16-4) / shr3
;       dq 0x8b8a898887868584, 0x030201008f8e8d8c ; shl 12 (16-4) / shr4
;       dq 0x8c8b8a8988878685, 0x04030201008f8e8d ; shl 11 (16-5) / shr5
;       dq 0x8d8c8b8a89888786, 0x0504030201008f8e ; shl 10 (16-6) / shr6
;       dq 0x8e8d8c8b8a898887, 0x060504030201008f ; shl 9  (16-7) / shr7
;       dq 0x8f8e8d8c8b8a8988, 0x0706050403020100 ; shl 8  (16-8) / shr8
;       dq 0x008f8e8d8c8b8a89, 0x0807060504030201 ; shl 7  (16-9) / shr9
;       dq 0x01008f8e8d8c8b8a, 0x0908070605040302 ; shl 6  (16-10) / shr10
;       dq 0x0201008f8e8d8c8b, 0x0a09080706050403 ; shl 5  (16-11) / shr11
;       dq 0x030201008f8e8d8c, 0x0b0a090807060504 ; shl 4  (16-12) / shr12
;       dq 0x04030201008f8e8d, 0x0c0b0a0908070605 ; shl 3  (16-13) / shr13
;       dq 0x0504030201008f8e, 0x0d0c0b0a09080706 ; shl 2  (16-14) / shr14
;       dq 0x060504030201008f, 0x0e0d0c0b0a090807 ; shl 1  (16-15) / shr15
dq 0x8786858483828100, 0x8f8e8d8c8b8a8988
dq 0x0706050403020100, 0x000e0d0c0b0a0908

;;;       func        core, ver, snum
slversion crc32_gzip_refl_by8, 01,   00,  002c
