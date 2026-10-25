PUBLIC RunLoop
EXTERN HandleExit:PROC
.CODE
RunLoop PROC
	; rcx = Adress of GuestVmcbPa 
	; args
  
	mov rsp, rcx
START:	
	mov rax, [rsp]
	vmload rax

	mov rax, [rsp]
	vmrun rax

	mov rax, [rsp]
	vmsave rax

	sub rsp, 300h	

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
	
	; 14x8 = 112 = 0x70
	; 0x70 + 0x300 shadow = 0x370
	; Vmcb* is 0x10 apart from GuestVmcbPa

	mov rdx, [rsp + 380h]
	mov r8, [rsp + 378h]
	lea r9, [rsp + 390h]

	sub rsp, 20h
	call HandleExit
	add rsp, 20h	

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

	add rsp, 300h

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

svm_vmsave PROC
    mov rax, rcx
    vmsave rax
    ret
svm_vmsave ENDP

svm_vmload PROC
    mov rax, rcx
    vmload rax
    ret
svm_vmload ENDP
END
