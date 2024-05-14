from pwn import *

if __name__ == "__main__":
    r = None
    r = remote(sys.argv[2], int(sys.argv[1]))
    r.recvuntil(b'to read it.')
    r.sendline(b'fortune000')
    r.sendline(b'flag')
    r.interactive()
    r.close()