; ASM functions currently have no prefix

.code

; Loads the IDT pointed to by rcx
LoadIDT PROC PUBLIC
lidt fword ptr[rcx]
ret
LoadIDT ENDP


; Triggers interrupt 8
IntEight PROC PUBLIC
int 8
ret
IntEight ENDP

end