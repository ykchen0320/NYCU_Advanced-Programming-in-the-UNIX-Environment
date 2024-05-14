#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import base64
import hashlib
import time
import sys
from pwn import *

def solve_pow(r):
    prefix = r.recvline().decode().split("'")[1];
    print(time.time(), "solving pow ...");
    solved = b''
    for i in range(1000000000):
        h = hashlib.sha1((prefix + str(i)).encode()).hexdigest();
        if h[:6] == '000000':
            solved = str(i).encode();
            print("solved =", solved);
            break;
    print(time.time(), "done.");
    r.sendlineafter(b'string S: ', base64.b64encode(solved));
def num_table(str2):    
    num = [' ┌───┐  │   │  │   │  │   │  └───┘ ', '  ─┐      │      │      │     ─┴─  ', ' ┌───┐      │  ┌───┘  │      └───┘ ', ' ┌───┐      │   ───┤      │  └───┘ ', ' │   │  │   │  └───┤      │      │ ', ' ┌────  │      └───┐      │  └───┘ ', ' ┌───┐  │      ├───┐  │   │  └───┘ ', ' ┌───┐  │   │      │      │      │ ', ' ┌───┐  │   │  ├───┤  │   │  └───┘ ', ' ┌───┐  │   │  └───┤      │  └───┘ ', '          │    ──┼──    │          ', '               ─────               ', '         ╲ ╱     ╳     ╱ ╲         ', '          •    ─────    •          ']
    for i in range (15):
        if str2 == num[i]:
            if i >= 10:
                if i == 10 :
                    return '+'
                if i == 11 :
                    return '-'
                if i == 12 :
                    return '*'
                if i == 13 :
                    return '/'
            return i
    return 123
def calculate(num):
    if(num[2] == '+'):
        ans = num[0] + num[1]
    if(num[2]=='-'):
        ans = num[0] - num[1]
    if(num[2] == '*'):
        ans = num[0] * num[1]
    if(num[2] == '/'):
        ans = num[0] / num[1]
    return int(ans)
def find_num(prefix2):
    str1 = [""] * 7
    for i in range(5):
        for j in range(49):
            #print(prefix2[i][j], end="")
            if j < 7:
                str1[0] = str1[0] + prefix2[i][j]
            if 7 <= j < 14:
                str1[1] = str1[1] + prefix2[i][j]
            if 14 <= j < 21:
                str1[2] = str1[2] + prefix2[i][j]
            if 21 <= j < 28:
                str1[3] = str1[3] + prefix2[i][j]
            if 28 <= j < 35:
                str1[4] = str1[4] + prefix2[i][j]
            if 35 <= j < 42:
                str1[5] = str1[5] + prefix2[i][j]
            if 42 <= j < 49:
                str1[6] = str1[6] + prefix2[i][j]
        #print()
    num = [0,0,'']
    count = 0
    for i in range(7):
        temp = num_table(str1[i])
        if temp != '+' and temp != '-' and temp != '*' and temp != '/':
            num[count] = 10 * num[count] + int(temp)
        else:
            num[2] = temp
            count += 1
    return(calculate(num))
def solve(r, challenge):
    temp = " " + str(challenge) + ": "
    r.recvuntil(temp.encode())
    temp = r.recvuntil(b' ').strip(b' ')
    prefix2 = base64.b64decode(temp).decode().split('\n')
    ans = find_num(prefix2)
    res = str(ans).encode()
    #print(res)
    r.sendlineafter(b'? ', res)
if __name__ == "__main__":
    r = None
    if len(sys.argv) == 2:
        r = remote('localhost', int(sys.argv[1]))
    elif len(sys.argv) == 3:
        r = remote(sys.argv[2], int(sys.argv[1]))
    else:
        r = process('./pow.py')
    solve_pow(r);
    r.recvuntil(b'Please complete the ')
    temp=r.recvuntil(b' ').decode().strip('')
    nums_of_challenges=int(temp)
    for i in range (1,nums_of_challenges+1):
        solve(r,i);
    r.interactive();
    r.close();

# vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4 number cindent fileencoding=utf-8 :
