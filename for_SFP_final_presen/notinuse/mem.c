#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char** argv) {
    if (argc <= 1) {
        fprintf(stderr, "共有メモリのshmidを引数に入力すること");
        return 1;
    }
    int shmid = atoi(argv[1]);  // 共有メモリのshmid
    char* addr;  // 自プロセス内で参照するメモリアドレス

    if ((addr = shmat(shmid, 0, 0)) == (void*)-1) {
        perror("shmat");
    }
    // 共有メモリ内の操作
    printf("%d\n", *addr);
    
}