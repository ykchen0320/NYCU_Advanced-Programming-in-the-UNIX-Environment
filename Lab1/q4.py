from pwn import *

if __name__ == '__main__':
    r = remote('ipinfo.io', 80)
    r.sendline(b'GET /ip HTTP/1.1')
    r.sendline(b'Host: ipinfo.io')
    r.sendline(b'')
    response = r.recvall(timeout = 1).decode().split('\n')
    ip_addr = response[-1]
    r.close()
    print(f"{response[-1]}")
