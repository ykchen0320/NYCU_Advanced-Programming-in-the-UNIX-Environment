#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
  int opt;
  char *sopath = "./logger.so", *filepath = NULL;
  while ((opt = getopt(argc, argv, "o:p:")) != -1) {
    switch (opt) {
      case 'o':
        filepath = optarg;
        break;
      case 'p':
        sopath = optarg;
        break;
      default:
        fprintf(stderr,
                "Usage: %s %s [-o file] [-p sopath] command [arg1 arg2 ...]\n",
                argv[0], argv[1]);
        exit(0);
    }
  }
  char ld_temp[30] = "LD_PRELOAD=";
  strcat(ld_temp, sopath);
  char *envp[] = {ld_temp, NULL};
  if (filepath) {
    FILE *file = fopen(filepath, "w");
    dup2(fileno(file), STDERR_FILENO);
    close(fileno(file));
  } else
    dup2(STDERR_FILENO, STDOUT_FILENO);
  execve(argv[optind + 1], &argv[optind + 1], envp);
  return 0;
}