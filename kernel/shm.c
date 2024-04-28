#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct shm_page {
    uint id;
    char *frame;
    int refcnt;
    int perm;
};

struct spinlock shm_table_lock;
struct shm_page shm_table[MAXSHM];

void shminit() {
    initlock(&shm_table_lock, "shmtb");
    acquire(&shm_table_lock);
    for (int i = 0; i < MAXSHM; i++) {
        shm_table[i].id = -1;
        shm_table[i].frame = 0;
        shm_table[i].refcnt = 0;
    }
    release(&shm_table_lock);
}

int shmget() {
    acquire(&shm_table_lock);
    for (int i = 0; i < MAXSHM; i++) {
        if (shm_table[i].id == -1) {
            if ((shm_table[i].frame = kalloc()) == 0) {
                release(&shm_table_lock);
                return -1;
            }
            shm_table[i].id = i;
            shm_table[i].refcnt = 0;
            memset(shm_table[i].frame, 0, PGSIZE);
            release(&shm_table_lock);
            return i;
        }
    }
    release(&shm_table_lock);
    return -1;
}

int shmattach(int id, char * addr) {
    struct proc *p = myproc();
    acquire(&shm_table_lock);
    if (id < 0 || id >= MAXSHM || shm_table[id].id == -1) {
        release(&shm_table_lock);
        return -1;
    }
    mappages(p->pagetable, (uint64)addr, PGSIZE, (uint64)shm_table[id].frame, PTE_W|PTE_U);
    shm_table[id].refcnt++;
    release(&shm_table_lock);
    return 0;
}

int shmdeattach(int id) {
    acquire(&shm_table_lock);
    if (id < 0 || id >= MAXSHM || shm_table[id].id == -1) {
        release(&shm_table_lock);
        return -1;
    }
    shm_table[id].refcnt--;
    if(shm_table[id].refcnt == 0) {
        kfree(shm_table[id].frame);
        shm_table[id].id = -1;
    }
    release(&shm_table_lock);
    return 0;
}

int shmqueryperm(int id) {
    acquire(&shm_table_lock);
    if (id < 0 || id >= MAXSHM || shm_table[id].id == -1) {
        release(&shm_table_lock);
        return -1;
    }
    int ret = shm_table[id].perm;
    release(&shm_table_lock);
    return ret;
}

int shmsetperm(int id, int perm) {
    acquire(&shm_table_lock);
    if (id < 0 || id >= MAXSHM || shm_table[id].id == -1) {
        release(&shm_table_lock);
        return -1;
    }
    shm_table[id].perm = perm;
    release(&shm_table_lock);
    return 0;
}