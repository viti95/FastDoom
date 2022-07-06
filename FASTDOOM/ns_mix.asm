BITS 32
%include "macros.inc"

extern _MV_HarshClipTable
extern _MV_MixDestination
extern _MV_MixPosition
extern _MV_LeftVolume
extern _MV_RightVolume
extern _MV_SampleSize
extern _MV_RightChannelOffset

BEGIN_CODE_SECTION

align 4

;================
;
; MV_Mix8BitMono
;
;================

; eax - position
; edx - rate
; ebx - start
; ecx - number of samples to mix

CODE_SYM_DEF MV_Mix8BitMono
; Two at once
        pushad

        mov     ebp, eax

        mov     esi, ebx ; Source pointer

        ; Volume table ptr
        mov     ebx,[_MV_LeftVolume] ; Since we're mono, use left volume
        mov     eax,apatch1+4
        mov     [eax],ebx
        mov     eax,apatch2+4
        mov     [eax],ebx

        ; Harsh Clip table ptr
        mov     ebx,[_MV_HarshClipTable]
        mov     eax,apatch3+3
        mov     [eax],ebx
        mov     eax,apatch4+3
        mov     [eax],ebx

        ; Rate scale ptr
        mov     eax,apatch5+2
        mov     [eax],edx
        mov     eax,apatch6+2
        mov     [eax],edx

        mov     edi,[_MV_MixDestination] ; Get the position to write to

        ; Number of samples to mix
        shr     ecx, 1 ; double sample count
        test    ecx, ecx
        je      short exit8M

;     eax - scratch
;     ebx - scratch
;     edx - scratch
;     ecx - count
;     edi - destination
;     esi - source
;     ebp - frac pointer
; apatch1 - volume table
; apatch2 - volume table
; apatch3 - harsh clip table
; apatch4 - harsh clip table
; apatch5 - sample rate
; apatch6 - sample rate

        mov     eax,ebp                         ; begin calculating first sample
        add     ebp,edx                         ; advance frac pointer
        shr     eax,16                          ; finish calculation for first sample

        mov     ebx,ebp                         ; begin calculating second sample
        add     ebp,edx                         ; advance frac pointer
        shr     ebx,16                          ; finish calculation for second sample

        movzx   eax, byte [esi+eax]             ; get first sample
        movzx   ebx, byte [esi+ebx]             ; get second sample

        xor     edx, edx

        align 4
mix8Mloop:
        mov     dl, byte [edi]                  ; get current sample from destination
apatch1:
        movsx   eax, byte [2*eax+0x12345678]    ; volume translate first sample
apatch2:
        movsx   ebx, byte [2*ebx+0x12345678]    ; volume translate second sample
apatch3:
        mov     eax, [eax + edx + 0x12345678]   ; mix first sample + harsh clip new sample
        mov     dl, byte [edi + 1]              ; get current sample from destination
        mov     [edi], al                       ; write new sample to destination        
        inc     edi                             ; move destination to second sample
apatch4:
        mov     ebx, [ebx + edx + 0x12345678]   ; mix second sample + harsh clip new sample
        mov     eax, ebp                        ; begin calculating third sample
        mov     [edi], bl                       ; write new sample to destination
        shr     eax, 16                         ; finish calculation for third sample        
apatch5:
        add     ebp, 0x12345678                 ; advance frac pointer
        mov     al, byte [esi+eax]              ; get third sample
        mov     ebx, ebp                        ; begin calculating fourth sample
        and     eax, 0x000000FF
        shr     ebx, 16                         ; finish calculation for fourth sample
apatch6:
        add     ebp, 0x12345678                 ; advance frac pointer
        mov     bl, byte [esi+ebx]              ; get fourth sample
        inc     edi                             ; move destination to third sample
        and     ebx, 0x000000FF
        dec     ecx                             ; decrement count
        jnz     short mix8Mloop                 ; loop

        mov     [_MV_MixDestination], edi       ; Store the current write position
        mov     [_MV_MixPosition], ebp          ; return position
exit8M:
        popad
        ret


;================
;
; MV_Mix8BitStereo
;
;================

; eax - position
; edx - rate
; ebx - start
; ecx - number of samples to mix

CODE_SYM_DEF MV_Mix8BitStereo
        pushad
        mov     ebp, eax

        mov     esi, ebx ; Source pointer

        ; Right channel offset
        mov     ebx, [_MV_RightChannelOffset]
        mov     eax, bpatch6+2
        mov     [eax],ebx
        mov     eax, bpatch7+2
        mov     [eax],ebx

        ; Volume table ptr
        mov     ebx, [_MV_LeftVolume]
        mov     eax, bpatch1+4
        mov     [eax],ebx

        mov     ebx, [_MV_RightVolume]
        mov     eax, bpatch2+4
        mov     [eax],ebx

        ; Rate scale ptr
        mov     eax, bpatch3+2
        mov     [eax],edx

        ; Harsh Clip table ptr
        mov     ebx, [_MV_HarshClipTable]
        mov     eax, bpatch4+2
        mov     [eax],ebx
        mov     eax, bpatch5+2
        mov     [eax],ebx

        mov     edi, [_MV_MixDestination] ; Get the position to write to

        ; Number of samples to mix
        test    ecx, ecx
        je      short exit8S

;     eax - scratch
;     ebx - scratch
;     edx - scratch
;     ecx - count
;     edi - destination
;     esi - source
;     ebp - frac pointer
; bpatch1 - left volume table
; bpatch2 - right volume table
; bpatch3 - sample rate
; bpatch4 - harsh clip table
; bpatch5 - harsh clip table

        mov     eax,ebp                     ; begin calculating first sample
        xor     ebx,ebx
        shr     eax,16                      ; finish calculation for first sample
        mov     bl, byte [esi+eax]          ; get first sample

        xor     edx, edx

        align 4
mix8Sloop:
bpatch1:
        movsx   eax, byte [2*ebx+0x12345678] ; volume translate left sample        
        mov     dl, byte [edi]              ; get current sample from destination
bpatch2:
        movsx   ebx, byte [2*ebx+0x12345678] ; volume translate right sample
        add     eax, edx                     ; mix left sample
bpatch3:
        add     ebp, 0x12345678              ; advance frac pointer
bpatch6:
        mov     dl, byte [edi+0x12345678]   ; get current sample from destination
bpatch4:
        mov     eax, [eax + 0x12345678]      ; harsh clip left sample
        add     ebx, edx                     ; mix right sample
        mov     [edi], al                    ; write left sample to destination
bpatch5:
        mov     ebx, [ebx + 0x12345678]      ; harsh clip right sample
        mov     eax, ebp                     ; begin calculating second sample
bpatch7:
        mov     [edi+0x12345678], bl         ; write right sample to destination
        shr     eax, 16                      ; finish calculation for second sample
        add     edi, 2                       ; move destination to second sample
        xor     ebx, ebx
        dec     ecx                          ; decrement count
        mov     bl, byte [esi+eax]          ; get second sample
        jnz     short mix8Sloop                    ; loop

        mov     [_MV_MixDestination], edi    ; Store the current write position
        mov     [_MV_MixPosition], ebp       ; return position

exit8S:
        popad
        ret

;================
;
; MV_Mix8BitStereo
;
;================

; eax - position
; edx - rate
; ebx - start
; ecx - number of samples to mix

CODE_SYM_DEF MV_Mix8BitUltrasound
        pushad
        mov     ebp, eax

        mov     esi, ebx ; Source pointer

        ; Right channel offset
        mov     ebx, [_MV_RightChannelOffset]
        mov     eax, cpatch6+2
        mov     [eax],ebx
        mov     eax, cpatch7+2
        mov     [eax],ebx

        ; Volume table ptr
        mov     ebx, [_MV_LeftVolume]
        mov     eax, cpatch1+4
        mov     [eax],ebx

        mov     ebx, [_MV_RightVolume]
        mov     eax, cpatch2+4
        mov     [eax],ebx

        ; Rate scale ptr
        mov     eax, cpatch3+2
        mov     [eax],edx

        ; Harsh Clip table ptr
        mov     ebx, [_MV_HarshClipTable]
        mov     eax, cpatch4+2
        mov     [eax],ebx
        mov     eax, cpatch5+2
        mov     [eax],ebx

        mov     edi, [_MV_MixDestination] ; Get the position to write to

        ; Number of samples to mix
        test    ecx, ecx
        je      short exit8S

;     eax - scratch
;     ebx - scratch
;     edx - scratch
;     ecx - count
;     edi - destination
;     esi - source
;     ebp - frac pointer
; bpatch1 - left volume table
; bpatch2 - right volume table
; bpatch3 - sample rate
; bpatch4 - harsh clip table
; bpatch5 - harsh clip table

        mov     eax,ebp                     ; begin calculating first sample
        xor     ebx,ebx
        shr     eax,16                      ; finish calculation for first sample
        mov     bl, byte [esi+eax]          ; get first sample

        xor     edx, edx

        align 4
mix8Uloop:
cpatch1:
        movsx   eax, byte [2*ebx+0x12345678] ; volume translate left sample        
        mov     dl, byte [edi]              ; get current sample from destination
cpatch2:
        movsx   ebx, byte [2*ebx+0x12345678] ; volume translate right sample
        add     eax, edx                     ; mix left sample
cpatch3:
        add     ebp, 0x12345678              ; advance frac pointer
cpatch6:
        mov     dl, byte [edi+0x12345678]   ; get current sample from destination
cpatch4:
        mov     eax, [eax + 0x12345678]      ; harsh clip left sample
        add     ebx, edx                     ; mix right sample
        mov     [edi], al                    ; write left sample to destination
cpatch5:
        mov     ebx, [ebx + 0x12345678]      ; harsh clip right sample
        mov     eax, ebp                     ; begin calculating second sample
cpatch7:
        mov     [edi+0x12345678], bl         ; write right sample to destination
        shr     eax, 16                      ; finish calculation for second sample
        inc     edi                          ; move destination to second sample
        xor     ebx, ebx
        dec     ecx                          ; decrement count
        mov     bl, byte [esi+eax]          ; get second sample
        jnz     short mix8Uloop                    ; loop

        mov     [_MV_MixDestination], edi    ; Store the current write position
        mov     [_MV_MixPosition], ebp       ; return position

exit8U:
        popad
        ret
