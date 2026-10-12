PUBLIC RunLoop
EXTERN HandleExit:PROC
.CODE
RunLoop PROC
	; rcx = VMCBPA, rdx = CustomRSP
  ; args
	add rsp, 98h
	
	pop rbx
	pop rdi
	pop rsi
	pop r12
	pop r13
	pop r14
	pop r15
	pop rbp

	mov r11, rcx
	mov rsp, rdx
START:
	mov rax, r11
	db 0Fh, 01h, 0DBh  ; vmsave
	
	mov rax, r11
  db 0Fh, 01h, 0D8h  ; vmrun

	mov rax, r11
	db 0Fh, 01h, 0DAh   ; vmload (obnoví host FS/GS/TR/LDTR/MSR z VMCB)	

	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov rcx, rsp
	sub rsp, 28h
	call HandleExit
	add rsp, 28h	

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx

	jmp START
	
	ret
RunLoop ENDP

WriteMSR PROC
  mov r8, rdx
  mov eax, r8d
  shr r8, 32
  mov edx, r8d
  mov ecx, ecx
  db 0Fh, 30h
  ret
WriteMSR ENDP

ReadCS PROC
	mov ax, cs
	ret
ReadCS ENDP

ReadSS PROC
	mov ax, ss
	ret
ReadSS ENDP

ReadGDT PROC
    sgdt [rcx]
    ret
ReadGDT ENDP

ReadIDT PROC
    sidt [rcx]
    ret
ReadIDT ENDP

ReadCR0 PROC
    mov rax, cr0
    ret
ReadCR0 ENDP

ReadCR4 PROC
    mov rax, cr4
    ret
ReadCR4 ENDP

ReadDR6 PROC
    mov rax, dr6
    ret
ReadDR6 ENDP

ReadDR7 PROC
    mov rax, dr7
    ret
ReadDR7 ENDP

ReadRSP PROC
    mov rax, rsp 
    ret
ReadRSP ENDP
END
