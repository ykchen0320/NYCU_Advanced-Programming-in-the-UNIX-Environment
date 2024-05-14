set -e
cd ~/NYCU_Advanced-Programming-in-the-UNIX-Environment/Lab2
cd rootfs/modules
chmod +x mazetest
cd ..
find  | cpio -o -H newc > rootfs.cpio
bzip2 -z rootfs.cpio
mv rootfs.cpio.bz2 ../dist
