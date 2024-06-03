#include <assert.h>
#include <capstone/capstone.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#define maxLength 1024
char programPath[maxLength] = "", cmdLine[maxLength] = "", cmd[4][maxLength];
pid_t childPid;
struct user_regs_struct regs;
static csh cshandle = 0;
long long breaklist[maxLength][2], breaking = 0, entry_point;
int enter = 0x01;
void cmd_slice() {
  if (cmdLine[strlen(cmdLine) - 1] == '\n') {
    cmdLine[strlen(cmdLine) - 1] = '\0';
  }
  char *token;
  int i = 0;
  token = strtok(cmdLine, " ");
  while (token != NULL) {
    if (i >= 4) break;
    strcpy(cmd[i], token);
    i++;
    token = strtok(NULL, " ");
  }
}
void break_in() {
  for (int i = 0; i < maxLength; i++) {
    if (!breaklist[i][0]) break;
    if (breaking && breaklist[i][0] == breaking) {
      continue;
    }
    ptrace(PTRACE_PEEKDATA, childPid, breaklist[i][0], NULL);
    long data = ptrace(PTRACE_PEEKDATA, childPid, breaklist[i][0], NULL);
    data = (data & 0xffffffffffffff00) | 0xcc;
    ptrace(PTRACE_POKEDATA, childPid, breaklist[i][0], data);
  }
}
void break_out() {
  for (int i = 0; i < maxLength; i++) {
    if (breaklist[i][0]) {
      long data = ptrace(PTRACE_PEEKDATA, childPid, breaklist[i][0], NULL);
      data = (data & 0xffffffffffffff00) | breaklist[i][1];
      ptrace(PTRACE_POKEDATA, childPid, breaklist[i][0], data);
    }
  }
}
void disassemble() {
  break_out();
  cs_open(CS_ARCH_X86, CS_MODE_64, &cshandle);
  size_t bufsz = 8;
  int addr_offset = 0;
  ptrace(PTRACE_GETREGS, childPid, NULL, &regs);
  for (int i = 0; i < 5; i++) {
    cs_insn *insn;
    long data = ptrace(PTRACE_PEEKDATA, childPid, regs.rip + addr_offset, NULL);
    if (!data) {
      printf("** the address is out of the range of the text section.\n");
      break;
    }
    unsigned char *buf = (unsigned char *)&data;
    cs_disasm(cshandle, (uint8_t *)buf, bufsz, regs.rip + addr_offset, 0,
              &insn);
    int j = 0;
    printf("      %lx: ", insn[0].address);
    for (int k = 0; k < 11; k++) {
      if (k < insn[0].size) {
        printf("%02x ", buf[j]);
        j++;
      } else {
        printf("   ");
      }
    }
    printf(" %s", insn[0].mnemonic);
    for (int k = 0; k < 10 - strlen(insn[0].mnemonic); k++) {
      printf(" ");
    }
    printf("%s\n", insn[0].op_str);
    addr_offset += insn[0].size;
  }
  break_in();
  cs_err cs_close(csh * handle);
}
void cmd_load() {
  childPid = fork();
  if (childPid == 0) {
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    char *args[] = {programPath, NULL};
    execv(programPath, args);
  } else {
    int status;
    if (waitpid(childPid, &status, 0) < 0) perror("wait error");
    assert(WIFSTOPPED(status));
    ptrace(PTRACE_SETOPTIONS, childPid, 0,
           PTRACE_O_EXITKILL | PTRACE_O_TRACESYSGOOD);
    ptrace(PTRACE_GETREGS, childPid, NULL, &regs);
    entry_point = regs.rip;
    printf("** program '%s' loaded. entry point 0x%llx.\n", programPath,
           regs.rip);
    disassemble();
  }
}
void cmd_si() {
  if (entry_point == regs.rip) {
    for (int i = 0; i < maxLength; i++) {
      if (breaklist[i][0] == entry_point) {
        long data = ptrace(PTRACE_PEEKDATA, childPid, entry_point, NULL);
        data = (data & 0xffffffffffffff00) | (breaklist[i][1]);
        ptrace(PTRACE_POKEDATA, childPid, entry_point, data);
        breaking = entry_point;
      }
    }
  }
  ptrace(PTRACE_SINGLESTEP, childPid, NULL, NULL);
  int status;
  waitpid(childPid, &status, 0);
  if (WIFEXITED(status)) {
    printf("** the target program terminated.\n");
    strcpy(programPath, "");
  } else if (WIFSTOPPED(status)) {
    ptrace(PTRACE_GETREGS, childPid, NULL, &regs);
    for (int i = 0; i < maxLength; i++) {
      if ((breaklist[i][0] == regs.rip)) {
        printf("** hit a breakpoint at 0x%llx.\n", regs.rip);
        breaking = regs.rip;
        break;
      } else {
        breaking = 0;
      }
    }
    disassemble();
  }
}
void cmd_cont() {
  if (breaking) {
    ptrace(PTRACE_SINGLESTEP, childPid, NULL, NULL);
    int status1;
    waitpid(childPid, &status1, 0);
    ptrace(PTRACE_PEEKDATA, childPid, breaking, NULL);
    long data = ptrace(PTRACE_PEEKDATA, childPid, breaking, NULL);
    data = (data & 0xffffffffffffff00) | 0xcc;
    ptrace(PTRACE_POKEDATA, childPid, breaking, data);
    breaking = 0;
  }
  ptrace(PTRACE_CONT, childPid, 0, 0);
  int status;
  waitpid(childPid, &status, 0);
  if (WIFEXITED(status)) {
    printf("** the target program terminated.\n");
    strcpy(programPath, "");
  } else if (WIFSTOPPED(status)) {
    ptrace(PTRACE_GETREGS, childPid, NULL, &regs);
    for (int i = 0; i < maxLength; i++) {
      if ((breaklist[i][0] == regs.rip - 1) && (breaking != regs.rip - 1)) {
        printf("** hit a breakpoint at 0x%llx.\n", --regs.rip);
        ptrace(PTRACE_SETREGS, childPid, NULL, &regs);
        breaking = regs.rip;
        break;
      } else {
        breaking = 0;
      }
    }
    disassemble();
  }
}
void cmd_info_reg() {
  printf("$rax 0x%016llx    $rbx 0x%016llx    $rcx 0x%016llx\n", regs.rax,
         regs.rbx, regs.rcx);
  printf("$rdx 0x%016llx    $rsi 0x%016llx    $rdi 0x%016llx\n", regs.rdx,
         regs.rsi, regs.rdi);
  printf("$rbp 0x%016llx    $rsp 0x%016llx    $r8  0x%016llx\n", regs.rbp,
         regs.rsp, regs.r8);
  printf("$r9  0x%016llx    $r10 0x%016llx    $r11 0x%016llx\n", regs.r8,
         regs.r9, regs.r11);
  printf("$r12 0x%016llx    $r13 0x%016llx    $r14 0x%016llx\n", regs.r12,
         regs.r13, regs.r14);
  printf("$r15 0x%016llx    $rip 0x%016llx    $eflags 0x%016llx\n", regs.r15,
         regs.rip, regs.eflags);
}
void cmd_info_break() {
  int flag = 0;
  for (int i = 0; i < maxLength; i++) {
    if (breaklist[i][0]) {
      if (!flag++) {
        printf("Num     Address\n");
      }
      printf("%d       0x%llx\n", i, breaklist[i][0]);
    }
  }
  if (!flag) {
    printf("** no breakpoints.\n");
  }
}
void cmd_break_point() {
  unsigned long long int addr = strtoull(cmd[1], NULL, 16);
  long data = ptrace(PTRACE_PEEKDATA, childPid, addr, NULL);
  for (int i = 0; i < maxLength; i++) {
    if (!breaklist[i][0] && !breaklist[i][1]) {
      breaklist[i][0] = addr;
      breaklist[i][1] = data & 0xff;
      break;
    }
  }
  data = (data & 0xffffffffffffff00) | 0xcc;
  ptrace(PTRACE_POKEDATA, childPid, addr, data);
  printf("** set a breakpoint at 0x%llx.\n", addr);
}
void cmd_delete_break_point() {
  if (breaklist[atoi(cmd[1])][0]) {
    long data =
        ptrace(PTRACE_PEEKDATA, childPid, breaklist[atoi(cmd[1])][0], NULL);
    data = (data & 0xffffffffffffff00) | (breaklist[atoi(cmd[1])][1]);
    ptrace(PTRACE_POKEDATA, childPid, breaklist[atoi(cmd[1])][0], data);
    breaklist[atoi(cmd[1])][0] = 0;
    printf("** delete breakpoint %d.\n", atoi(cmd[1]));
  } else {
    printf("** breakpoint %d does not exist.\n", atoi(cmd[1]));
  }
}
void cmd_patch_memory() {
  break_out();
  unsigned long long int addr = strtoull(cmd[1], NULL, 16);
  long data = ptrace(PTRACE_PEEKDATA, childPid, addr, NULL);
  if (atoi(cmd[3]) == 1) {
    data = (data & 0xffffffffffffff00) | strtol(cmd[2], NULL, 16);
  } else if (atoi(cmd[3]) == 2) {
    data = (data & 0xffffffffffff0000) | strtol(cmd[2], NULL, 16);
  } else if (atoi(cmd[3]) == 4) {
    data = (data & 0xffffffff00000000) | strtol(cmd[2], NULL, 16);
  } else if (atoi(cmd[3]) == 8) {
    data = (data & 0x0000000000000000) | strtol(cmd[2], NULL, 16);
  }
  ptrace(PTRACE_POKEDATA, childPid, addr, data);
  printf("** patch memory at address 0x%llx.\n", addr);
  break_in();
}
void cmd_syscall() {
  ptrace(PTRACE_SYSCALL, childPid, 0, 0);
  int status;
  waitpid(childPid, &status, 0);
  if (WIFEXITED(status)) {
    printf("** the target program terminated.\n");
    strcpy(programPath, "");
  } else if (WIFSTOPPED(status)) {
    ptrace(PTRACE_GETREGS, childPid, NULL, &regs);
    if (WSTOPSIG(status) & 0x80) {
      if (enter) {
        regs.rip -= 2;
        ptrace(PTRACE_SETREGS, childPid, NULL, &regs);
        printf("** enter a syscall(%lld) at 0x%llx.\n", regs.orig_rax,
               regs.rip);
      } else {
        printf("** leave a syscall(%lld) = %lld at 0x%llx.\n", regs.orig_rax,
               regs.rax, regs.rip);
      }
      enter ^= 0x01;
    } else {
      for (int i = 0; i < maxLength; i++) {
        if ((breaklist[i][0] == regs.rip - 1) && (breaking != regs.rip - 1)) {
          printf("** hit a breakpoint at 0x%llx.\n", --regs.rip);
          ptrace(PTRACE_SETREGS, childPid, NULL, &regs);
          breaking = regs.rip;
          break;
        } else {
          breaking = 0;
        }
      }
    }
    disassemble();
    if (WSTOPSIG(status) & 0x80 && enter) {
      regs.rip += 2;
      ptrace(PTRACE_SETREGS, childPid, NULL, &regs);
    }
  }
}
void exec_sdb() {
  for (int i = 0; i < maxLength; i++) {
    breaklist[i][0] = 0;
    breaklist[i][1] = 0;
  }
  while (1) {
    printf("(sdb) ");
    fgets(cmdLine, maxLength, stdin);
    cmd_slice();
    if (!strcmp(programPath, "") && strcmp(cmd[0], "load")) {
      printf("** please load a program first.\n");
      continue;
    }
    if (!strcmp(cmd[0], "load")) {
      strcpy(programPath, cmd[1]);
      cmd_load();
    } else if (!strcmp(cmd[0], "si")) {
      cmd_si();
    } else if (!strcmp(cmd[0], "cont")) {
      cmd_cont();
    } else if (!strcmp(cmd[0], "info")) {
      if (!strcmp(cmd[1], "reg")) {
        cmd_info_reg();
      } else if (!strcmp(cmd[1], "break")) {
        cmd_info_break();
      }
    } else if (!strcmp(cmd[0], "break")) {
      cmd_break_point();
    } else if (!strcmp(cmd[0], "delete")) {
      cmd_delete_break_point();
    } else if (!strcmp(cmd[0], "patch")) {
      cmd_patch_memory();
    } else if (!strcmp(cmd[0], "syscall")) {
      cmd_syscall();
    }
    strcpy(cmd[0], "");
  }
}
int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  if (argc <= 2) {
    if (argc == 2) {
      strcpy(programPath, argv[1]);
      cmd_load();
    }
    exec_sdb();
  } else {
    printf("Usage: ./sdb (without any argument)\nUsage: ./sdb [program]\n");
  }
  return 0;
}