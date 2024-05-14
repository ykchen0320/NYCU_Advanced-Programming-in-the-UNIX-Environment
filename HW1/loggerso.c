#define _GNU_SOURCE
#define buffer_size 1024
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
char realname[buffer_size], filename[50], fileshortname[50];
char blacklist_file[] = "config.txt";
FILE *(*origin_fopen)(const char *pathname, const char *mode);
size_t (*origin_fread)(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t (*origin_fwrite)(const void *ptr, size_t size, size_t nmemb,
                        FILE *stream);
int (*origin_connect)(int sockfd, const struct sockaddr *addr,
                      socklen_t addrlen);
int (*origin_getaddrinfo)(const char *node, const char *service,
                          const struct addrinfo *hints, struct addrinfo **res);
int (*origin_system)(const char *command);
int check_blacklist(char *find, const char *str) {
  origin_fopen = dlsym(RTLD_NEXT, "fopen");
  FILE *file = origin_fopen(blacklist_file, "r");
  char **blacklist = malloc(sizeof(char *) * 20);
  for (int i = 0; i < 20; i++) blacklist[i] = NULL;
  char start[30] = "BEGIN ";
  char end[30] = "END ";
  strcat(start, find);
  strcat(end, find);
  strcat(start, "-blacklist\n");
  strcat(end, "-blacklist\n");
  char temp[buffer_size];
  while (fgets(temp, buffer_size, file)) {
    int i = 0;
    if (strcmp(temp, start)) continue;
    while (fgets(temp, buffer_size, file)) {
      if (!strcmp(temp, end)) break;
      blacklist[i] = malloc(sizeof(char *) * buffer_size);
      for (int j = 0; j < sizeof(temp) / sizeof(char); j++) {
        if (temp[j] == '\n') {
          blacklist[i][j] = '\0';
          break;
        }
        blacklist[i][j] = temp[j];
      }
      i++;
    }
    if (!strcmp(temp, end)) break;
  }
  int permission = 1;
  for (int i = 0; i < 20; i++) {
    if (!blacklist[i]) break;
    permission = 0;
    for (int j = 0, k = 0; j < strlen(blacklist[i]), k < strlen(str);
         j++, k++) {
      if (blacklist[i][j] == '*') {
        while (j < strlen(blacklist[i]) && blacklist[i][j] != '/') j++;
        j--;
        while (k < strlen(str) && str[k] != '/') k++;
        k--;
        continue;
      }
      if (blacklist[i][j] != str[k]) {
        permission = 1;
        break;
      }
    }
    if (!permission) break;
  }
  fclose(file);
  free(blacklist);
  return permission;
}
void set_file(const char *pathname) {
  int count = 0;
  for (int i = 0; i < strlen(pathname); i++) {
    if (pathname[i] == '/') count = i;
  }
  strcpy(filename, pathname + count + 1);
  for (int i = 0; i < strlen(filename); i++) {
    if (filename[i] == '.') {
      strncpy(fileshortname, filename, i);
      fileshortname[i] = '\0';
      break;
    }
  }
  return;
}
FILE *fopen(const char *pathname, const char *mode) {
  realpath(pathname, realname);
  strcpy(realname, realname + 4);
  int flag = check_blacklist("open", realname);
  set_file(realname);
  if (flag) {
    origin_fopen = dlsym(RTLD_NEXT, "fopen");
    FILE *file = origin_fopen(pathname, mode);
    fprintf(stderr, "[logger] fopen(\"%s\", \"%s\") = %p\n", pathname, mode,
            file);
    return file;
  } else {
    errno = EACCES;
    fprintf(stderr, "[logger] fopen(\"%s\", \"%s\") = 0x0\n", pathname, mode);
    return NULL;
  }
}
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  size_t ret = 0;
  pid_t pid = getpid();
  char pid_str[50];
  sprintf(pid_str, "%d", pid);
  char logname[50] = "";
  strcat(logname, pid_str);
  strcat(logname, "-");
  strcat(logname, fileshortname);
  strcat(logname, "-read.log");
  FILE *file = origin_fopen(blacklist_file, "r");
  char *blacklist = malloc(sizeof(char) * buffer_size);
  while (fgets(blacklist, buffer_size, file)) {
    int i = 0;
    if (strcmp(blacklist, "BEGIN read-blacklist\n")) continue;
    fgets(blacklist, buffer_size, file);
    fclose(file);
    break;
  }
  for (int i = 0; i < strlen(blacklist); i++)
    if (blacklist[i] == '\n') blacklist[i] = '\0';
  file = origin_fopen(filename, "r");
  char content[buffer_size];
  int flag = 1;
  while (fgets(content, buffer_size, file)) {
    for (int i = 0, j = 0; i < strlen(content) && j < strlen(blacklist); i++) {
      if (content[i] == blacklist[j])
        j++;
      else
        j = 0;
      if (j == strlen(blacklist)) {
        flag = 0;
        break;
      }
    }
    if (!flag) break;
  }
  free(blacklist);
  blacklist = NULL;
  file = origin_fopen(logname, "a");
  if (flag) {
    origin_fread = dlsym(RTLD_NEXT, "fread");
    ret = origin_fread(ptr, size, nmemb, stream);
    char read_str[buffer_size];
    strncpy(read_str, ptr, nmemb);
    read_str[nmemb] = '\0';
    fprintf(file, "%s\n", read_str);
    fprintf(stderr, "[logger] fread(\"%p\", %ld, %ld, %p) = %ld\n", ptr, size,
            nmemb, stream, ret);
    fclose(file);
    return ret;
  } else {
    ret = 0;
    errno = EACCES;
    fprintf(stderr, "[logger] fread(\"%p\", %ld, %ld, %p) = %ld\n", ptr, size,
            nmemb, stream, ret);
    fclose(file);
    return 0;
  }
}
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  int flag = check_blacklist("write", realname);
  size_t ret = 0;
  pid_t pid = getpid();
  char pid_str[buffer_size];
  sprintf(pid_str, "%d", pid);
  char logname[buffer_size] = "";
  strcat(logname, pid_str);
  strcat(logname, "-");
  strcat(logname, fileshortname);
  strcat(logname, "-write.log");
  char str[buffer_size];
  int slash_handle = 0;
  for (int i = 0, j = 0; i < buffer_size, j < strlen(ptr); i++, j++) {
    if (((char *)ptr)[j] == '\n') {
      str[i] = '\\';
      i++;
      if (j < nmemb) slash_handle++;
      str[i] = 'n';
    } else
      str[i] = ((char *)ptr)[j];
  }
  origin_fopen = dlsym(RTLD_NEXT, "fopen");
  FILE *file = origin_fopen(logname, "a");
  if (flag) {
    origin_fwrite = dlsym(RTLD_NEXT, "fwrite");
    ret = origin_fwrite(ptr, size, nmemb, stream);
    fprintf(stderr, "[logger] fwrite(\"%s\", %ld, %ld, %p) = %ld\n", str, size,
            nmemb, stream, ret);
    str[nmemb + slash_handle] = '\0';
    fprintf(file, "%s\n", str);
    fclose(file);
    return ret;
  } else {
    errno = EACCES;
    fprintf(stderr, "[logger] fwrite(\"%s\", %ld, %ld, %p) = %d\n", str, size,
            nmemb, stream, 0);
    fclose(file);
    return 0;
  }
}
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  const struct sockaddr_in *addr_in = (const struct sockaddr_in *)addr;
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr_in->sin_addr), ip, INET_ADDRSTRLEN);
  int flag = check_blacklist("connect", ip), ret = -1;
  if (flag) {
    origin_connect = dlsym(RTLD_NEXT, "connect");
    ret = origin_connect(sockfd, addr, addrlen);
  }
  fprintf(stderr, "[logger] connect(%d, \"%s\", %d) = %d\n", sockfd, ip,
          addrlen, ret);
  if (flag)
    return ret;
  else {
    errno = ECONNREFUSED;
    return -1;
  }
}
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
  int flag = check_blacklist("getaddrinfo", node), ret = -1;
  if (flag) {
    origin_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");
    ret = origin_getaddrinfo(node, service, hints, res);
  }
  fprintf(stderr, "[logger] getaddrinfo(\"%s\" , %s, %p, %p) = %d\n", node,
          service, hints, res, ret);
  if (flag)
    return ret;
  else
    return EAI_NONAME;
}
int system(const char *command) {
  origin_system = dlsym(RTLD_NEXT, "system");
  int ret = origin_system(command);
  fprintf(stderr, "[logger] system(\"%s\") = %d\n", command, ret);
  return ret;
}
