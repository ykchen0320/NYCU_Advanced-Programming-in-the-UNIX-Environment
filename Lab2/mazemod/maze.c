/*
 * Lab problem set for UNIX programming course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include "maze.h"

#include <linux/cdev.h>
#include <linux/cred.h>  // for current_uid();
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>    // included for __init and __exit macros
#include <linux/kernel.h>  // included for KERN_INFO
#include <linux/module.h>  // included for all kernel modules
#include <linux/proc_fs.h>
#include <linux/sched.h>  // task_struct requried for current_uid()
#include <linux/seq_file.h>
#include <linux/slab.h>  // for kmalloc/kfree
#include <linux/string.h>
#include <linux/uaccess.h>  // copy_to_user
static dev_t devnum;
static struct cdev c_dev;
static struct class *clazz;
typedef struct {
  coord_t curr;
  maze_t maze;
  int pid;
} user_t;
int userid[_MAZE_MAXUSER] = {0};
user_t *user[_MAZE_MAXUSER] = {NULL};
int nums_of_user = 0;
DEFINE_MUTEX(mutex1);
DEFINE_MUTEX(mutex2);
int find_uid(void) {
  for (int i = 0; i < _MAZE_MAXUSER; i++) {
    if (current->pid == userid[i]) return i;
  }
  return -1;
}
void DFS(int x, int y, int *visit) {
  int unum = find_uid();
  visit[y * user[unum]->maze.w + x] = 1;
  coord_t dir[] = {{-2, 0}, {2, 0}, {0, -2}, {0, 2}};
  unsigned int rand1;
  int table[4] = {0};
  while (1) {
    rand1 = get_random_u32() % 4;
    if (table[0] && table[1] && table[2] && table[3]) break;
    if (y + dir[rand1].y < 1 || y + dir[rand1].y > user[unum]->maze.h - 1 ||
        x + dir[rand1].x < 1 || x + dir[rand1].x > user[unum]->maze.w - 1) {
      table[rand1] = 1;
      continue;
    }
    if (!(visit[(y + dir[rand1].y) * user[unum]->maze.w + x + dir[rand1].x])) {
      user[unum]->maze.blk[y + (dir[rand1].y / 2)][x + (dir[rand1].x / 2)] =
          '.';
      table[rand1] = 1;
      DFS(x + dir[rand1].x, y + dir[rand1].y, visit);
    }
    table[rand1] = 1;
  }
  return;
}
static int mazemod_dev_open(struct inode *i, struct file *f) { return 0; }

static int mazemod_dev_close(struct inode *i, struct file *f) {
  int unum = find_uid();
  if (unum < 0) return 0;
  kfree(user[unum]);
  user[unum] = NULL;
  mutex_lock(&mutex1);
  userid[unum] = 0;
  nums_of_user--;
  mutex_unlock(&mutex1);
  return 0;
}

static ssize_t mazemod_dev_read(struct file *f, char __user *buf, size_t len,
                                loff_t *off) {
  int unum = find_uid();
  if (unum < 0) return -EBADFD;
  len = user[unum]->maze.h * user[unum]->maze.w;
  unsigned char *temp;
  temp = kzalloc(len, GFP_KERNEL);
  for (int i = 0; i < user[unum]->maze.h; i++) {
    for (int j = 0; j < user[unum]->maze.w; j++) {
      if (user[unum]->maze.blk[i][j] == '#')
        *(temp + (i * user[unum]->maze.w + j)) = (unsigned char)1;
      else
        *(temp + (i * user[unum]->maze.w + j)) = (unsigned char)0;
    }
  }
  if (copy_to_user(buf, temp, sizeof(unsigned char) * len)) {
    kfree(temp);
    return -EBUSY;
  }
  kfree(temp);
  return len;
}

static ssize_t mazemod_dev_write(struct file *f, const char __user *buf,
                                 size_t len, loff_t *off) {
  int unum = find_uid();
  if (unum < 0) return -EBADFD;
  if (len % sizeof(coord_t)) return -EINVAL;
  coord_t seq[64];
  if (copy_from_user(seq, buf, len)) return -EBUSY;
  for (int i = 0; i < 64; i++) {
    if (user[unum]->maze.blk[user[unum]->curr.y + seq[i].y]
                            [user[unum]->curr.x + seq[i].x] != '#') {
      user[unum]->maze.blk[user[unum]->curr.y][user[unum]->curr.x] = '.';
      user[unum]->curr.y = user[unum]->curr.y + seq[i].y;
      user[unum]->curr.x = user[unum]->curr.x + seq[i].x;
      user[unum]->maze.blk[user[unum]->curr.y][user[unum]->curr.x] = '*';
    }
  }
  return len;
}

static long mazemod_dev_ioctl(struct file *fp, unsigned int cmd,
                              unsigned long arg) {
  coord_t *temp = NULL;
  int unum;
  switch (cmd) {
    case MAZE_CREATE:
      unum = find_uid();
      if (find_uid() >= 0) return -EEXIST;
      if (nums_of_user >= _MAZE_MAXUSER) return -ENOMEM;
      temp = kzalloc(sizeof(coord_t), GFP_KERNEL);
      if (copy_from_user(temp, (coord_t *)arg, sizeof(coord_t))) return -EBUSY;
      if (temp->x > _MAZE_MAXX || temp->y > _MAZE_MAXY || temp->x == -1 ||
          temp->y == -1)
        return -EINVAL;
      mutex_lock(&mutex1);
      unum = nums_of_user;
      userid[nums_of_user] = current->pid;
      nums_of_user++;
      mutex_unlock(&mutex1);
      user[unum] = kzalloc(sizeof(user_t), GFP_KERNEL);
      user[unum]->pid = current->pid;
      user[unum]->maze.w = temp->x;
      user[unum]->maze.h = temp->y;
      kfree(temp);
      temp = NULL;
      for (int i = 0; i < user[unum]->maze.h; i++) {
        for (int j = 0; j < user[unum]->maze.w; j++) {
          if (i % 2 && j % 2)
            user[unum]->maze.blk[i][j] = '.';
          else
            user[unum]->maze.blk[i][j] = '#';
        }
      }
      unsigned int randx = 0, randy = 0;
      while (!(randx % 2)) randx = get_random_u32() % user[unum]->maze.w;
      while (!(randy % 2)) randy = get_random_u32() % user[unum]->maze.h;
      user[unum]->maze.sx = randx;
      user[unum]->maze.sy = randy;
      user[unum]->curr.x = user[unum]->maze.sx;
      user[unum]->curr.y = user[unum]->maze.sy;
      user[unum]->maze.blk[user[unum]->maze.sy][user[unum]->maze.sx] = 'S';
      user[unum]->maze.blk[user[unum]->maze.sy][user[unum]->maze.sx] = '*';
      mutex_lock(&mutex2);
      int *visit = kzalloc(
          sizeof(int) * user[unum]->maze.h * user[unum]->maze.w, GFP_KERNEL);
      DFS(user[unum]->maze.sx, user[unum]->maze.sy, visit);
      kfree(visit);
      mutex_unlock(&mutex2);
      randx = 0, randy = 0;
      while (!(randx % 2)) randx = get_random_u32() % user[unum]->maze.w;
      while ((!(randy % 2)) &&
             !(randx == user[unum]->maze.sx && randy == user[unum]->maze.sy))
        randy = get_random_u32() % user[unum]->maze.h;
      user[unum]->maze.ex = randx;
      user[unum]->maze.ey = randy;
      user[unum]->maze.blk[user[unum]->maze.ey][user[unum]->maze.ex] = 'E';
      break;
    case MAZE_RESET:
      unum = find_uid();
      if (!user[unum]) return -ENOENT;
      user[unum]->curr.x = user[unum]->maze.sx;
      user[unum]->curr.y = user[unum]->maze.sy;
      user[unum]->maze.blk[user[unum]->maze.sy][user[unum]->maze.sx] = '*';
      break;
    case MAZE_DESTROY:
      unum = find_uid();
      if (!user[unum]) return -ENOENT;
      kfree(user[unum]);
      user[unum] = NULL;
      mutex_lock(&mutex1);
      userid[unum] = 0;
      nums_of_user--;
      mutex_unlock(&mutex1);
      break;
    case MAZE_GETSIZE:
      unum = find_uid();
      if (!user[unum]) return -ENOENT;
      temp = kzalloc(sizeof(coord_t), GFP_KERNEL);
      temp->x = user[unum]->maze.w;
      temp->y = user[unum]->maze.h;
      if (copy_to_user((coord_t *)arg, temp, sizeof(temp))) {
        kfree(temp);
        temp = NULL;
        return -EBUSY;
      }
      kfree(temp);
      temp = NULL;
      break;
    case MAZE_MOVE:
      unum = find_uid();
      if (!user[unum]) return -ENOENT;
      coord_t mv;
      if (copy_from_user(&mv, (coord_t *)arg, sizeof(coord_t))) return -EBUSY;
      if (user[unum]->maze.blk[user[unum]->curr.y + mv.y]
                              [user[unum]->curr.x + mv.x] != '#') {
        user[unum]->maze.blk[user[unum]->curr.y][user[unum]->curr.x] = '.';
        user[unum]->curr.y = user[unum]->curr.y + mv.y;
        user[unum]->curr.x = user[unum]->curr.x + mv.x;
        user[unum]->maze.blk[user[unum]->curr.y][user[unum]->curr.x] = '*';
      } else
        return 0;
      break;
    case MAZE_GETPOS:
      unum = find_uid();
      if (!user[unum]) return -ENOENT;
      if (copy_to_user((coord_t *)arg, &user[unum]->curr, sizeof(coord_t)))
        return -EBUSY;
      break;
    case MAZE_GETSTART:
      unum = find_uid();
      if (!user[unum]) return -ENOENT;
      temp = kzalloc(sizeof(coord_t), GFP_KERNEL);
      temp->x = user[unum]->maze.sx;
      temp->y = user[unum]->maze.sy;
      if (copy_to_user((coord_t *)arg, temp, sizeof(coord_t))) {
        kfree(temp);
        temp = NULL;
        return -EBUSY;
      }
      kfree(temp);
      temp = NULL;
      break;
    case MAZE_GETEND:
      unum = find_uid();
      if (!user[unum]) return -ENOENT;
      temp = kzalloc(sizeof(coord_t), GFP_KERNEL);
      temp->x = user[unum]->maze.ex;
      temp->y = user[unum]->maze.ey;
      if (copy_to_user((coord_t *)arg, temp, sizeof(coord_t))) {
        kfree(temp);
        temp = NULL;
        return -EBUSY;
      }
      kfree(temp);
      temp = NULL;
      break;
    default:
      printk(KERN_INFO "mazemod: ioctl cmd = %u arg = %lu.\n", cmd, arg);
      break;
  }
  return 0;
}

static const struct file_operations mazemod_dev_fops = {
    .owner = THIS_MODULE,
    .open = mazemod_dev_open,
    .read = mazemod_dev_read,
    .write = mazemod_dev_write,
    .unlocked_ioctl = mazemod_dev_ioctl,
    .release = mazemod_dev_close};

static int mazemod_proc_read(struct seq_file *m, void *v) {
  int i = 0;
  while (i < 3) {
    if (!user[i])
      seq_printf(m, "#%02d: vacancy\n\n", i);
    else {
      seq_printf(m,
                 "#%02d: pid %d - [%d x %d]: (%d, %d) -> (%d, %d) @ (%d, %d)\n",
                 i, user[i]->pid, user[i]->maze.w, user[i]->maze.h,
                 user[i]->maze.sx, user[i]->maze.sy, user[i]->maze.ex,
                 user[i]->maze.ey, user[i]->curr.x, user[i]->curr.y);
      for (int j = 0; j < user[i]->maze.h; j++)
        seq_printf(m, "- %03d: %s\n", j, user[i]->maze.blk[j]);
    }
    i++;
  }
  return 0;
}

static int mazemod_proc_open(struct inode *inode, struct file *file) {
  return single_open(file, mazemod_proc_read, NULL);
}

static const struct proc_ops mazemod_proc_fops = {
    .proc_open = mazemod_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static char *mazemod_devnode(const struct device *dev, umode_t *mode) {
  if (mode == NULL) return NULL;
  *mode = 0666;
  return NULL;
}

static int __init mazemod_init(void) {
  // create char dev
  if (alloc_chrdev_region(&devnum, 0, 1, "updev") < 0) return -1;
  if ((clazz = class_create("upclass")) == NULL) goto release_region;
  clazz->devnode = mazemod_devnode;
  if (device_create(clazz, NULL, devnum, NULL, "maze") == NULL)
    goto release_class;
  cdev_init(&c_dev, &mazemod_dev_fops);
  if (cdev_add(&c_dev, devnum, 1) == -1) goto release_device;

  // create proc
  proc_create("maze", 0, NULL, &mazemod_proc_fops);

  printk(KERN_INFO "maze: initialized.\n");
  return 0;  // Non-zero return means that the module couldn't be loaded.

release_device:
  device_destroy(clazz, devnum);
release_class:
  class_destroy(clazz);
release_region:
  unregister_chrdev_region(devnum, 1);
  return -1;
}

static void __exit mazemod_cleanup(void) {
  remove_proc_entry("maze", NULL);

  cdev_del(&c_dev);
  device_destroy(clazz, devnum);
  class_destroy(clazz);
  unregister_chrdev_region(devnum, 1);

  printk(KERN_INFO "maze: cleaned up.\n");
}

module_init(mazemod_init);
module_exit(mazemod_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chun-Ying Huang");
MODULE_DESCRIPTION("The unix programming course demo kernel module.");
