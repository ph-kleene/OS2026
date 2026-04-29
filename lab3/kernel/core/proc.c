#include "proc.h"
#include "device.h"

/* 仅在“vfork共享内存专项测试”时改为1；平时保持0避免影响常规测试。 */
#define VFORK_MEMORY_TEST_MODE 0

struct PCB pcb[MAX_PCB];
struct PCB *current = NULL;

void schedule() {
    /* TODO 1: 实现逆向扫描策略
     * - 从pcb[MAX_PCB-1]向pcb[0]扫描
     * - 查找第一个RUNNABLE或RUNNING状态的进程
     * - 实现高PID优先的调度策略
     */
    struct PCB *next = NULL;
    for (int i = MAX_PCB - 1; i >= 0; i--) {
        if (pcb[i].state == RUNNABLE || pcb[i].state == RUNNING) {
            next = &pcb[i];
            break;
        }
    }

    if (next == NULL) {
        next = &pcb[0];
    }


    /* TODO 2: 上下文切换核心逻辑
     * - 若有进程切换：
     *   - 将当前进程state从RUNNING改为RUNNABLE
     *   - 设置next进程state为RUNNING
     *   - 更新current指针
     *   - 更新TSS.esp0指向新进程内核栈顶
     *   - 调用update_user_seg_base切换GDT基址
     */
    if (next != current) {
        if (current != NULL && current->state == RUNNING) {
            current->state = RUNNABLE;
        }

        next->state = RUNNING;
        current = next;

        extern TSS tss;
        tss.esp0 = (uint32_t)current->kstack + KSTACK_SIZE;
        update_user_seg_base(current->mem_base, current->mem_limit);
    }
}

int do_fork() {
    /* TODO 1: 查找空闲PCB槽位
     * - 遍历pcb[1]到pcb[MAX_PCB-1]
     * - 找到第一个state==UNUSED的进程
     * - 若无空闲则返回-1
     */
    int child_idx = -1;
    for (int i = 1; i < MAX_PCB; i++) {
        if (pcb[i].state == UNUSED) {
            child_idx = i;
            break;
        }
    }
    if (child_idx == -1) return -1;

    struct PCB *child = &pcb[child_idx];
    struct PCB *parent = current;

    /* TODO 2: 复制128KB物理内存槽位
     * - 使用SLOT_SIZE循环拷贝
     * - 从父进程mem_base复制到子进程mem_base
     */
    uint8_t *src = (uint8_t *)parent->mem_base;
    uint8_t *dst = (uint8_t *)child->mem_base;
    for (uint32_t i = 0; i < SLOT_SIZE; i++) {
        dst[i] = src[i];
    }

    /* TODO 3: 复制内核栈和TrapFrame
     * - 拷贝KSTACK_SIZE字节的内核栈
     * - 重新计算子进程tf相对于kstack的偏移
     */
    uint32_t offset = (uint32_t)parent->tf - (uint32_t)parent->kstack;
    for (uint32_t i = 0; i < KSTACK_SIZE; i++) {
        child->kstack[i] = parent->kstack[i];
    }
    child->tf = (struct TrapFrame *)((uint32_t)child->kstack + offset);

    /* TODO 4: 调整子进程TrapFrame
     * - 设置子进程eax=0（fork返回值：子进程返回0）
     * - 设置pid和ppid
     * - 设置state=RUNNABLE
     * - 设置vfork_count=0
     */
    child->tf->eax = 0;
    child->pid = child_idx;
    child->ppid = parent->pid;
    child->state = RUNNABLE;
    child->sleep_ticks = 0;
    child->mem_limit = SEG_LIMIT;
    child->vfork_count = 0;

    schedule();
    return child->pid;
}

int do_vfork() {
    /* TODO 1: 查找空闲PCB槽位（与fork相同） */
    int child_idx = -1;
    for (int i = 1; i < MAX_PCB; i++) {
        if (pcb[i].state == UNUSED) {
            child_idx = i;
            break;
        }
    }
    if (child_idx == -1) return -1;

    struct PCB *parent = current;

    /* TODO 2: 检查vfork并发控制
     * - 父进程vfork_count++
     * - 若vfork_count>1则失败回退，返回-1
     * - 确保同一时间父进程只有一个未exec的vfork子进程
     */
    parent->vfork_count++;
    if (parent->vfork_count > 1) {
        parent->vfork_count--;
        return -1;
    }

    struct PCB *child = &pcb[child_idx];

    /* TODO 3: 设置vfork特殊属性——共享内存
     * - 子进程mem_base临时指向父进程mem_base（区别于fork的独立拷贝）
     * - 子进程栈顶偏移VFORK_STACK_OFFSET(32KB)避免栈冲突
     */
    child->mem_base = parent->mem_base;
    child->mem_limit = parent->mem_limit;

    /* TODO 4: 复制TrapFrame（与fork类似但内存共享）
     * - 拷贝内核栈
     * - 调整子进程esp_user
     * - 设置子进程eax=0
     */
    uint32_t offset = (uint32_t)parent->tf - (uint32_t)parent->kstack;
    for (uint32_t i = 0; i < KSTACK_SIZE; i++) {
        child->kstack[i] = parent->kstack[i];
    }
    child->tf = (struct TrapFrame *)((uint32_t)child->kstack + offset);
    child->tf->esp_user -= VFORK_STACK_OFFSET;
    child->tf->eax = 0;

#if VFORK_MEMORY_TEST_MODE
    if (parent->pid > 0) {
        parent->state = BLOCKED;
        parent->tf->esp_user -= VFORK_STACK_OFFSET;

        uint32_t used = (SEG_LIMIT + 1) - (parent->tf->esp_user + VFORK_STACK_OFFSET);
        uint8_t *stack_src = (uint8_t *)(parent->mem_base + parent->tf->esp_user + VFORK_STACK_OFFSET);
        uint8_t *stack_dst = (uint8_t *)(parent->mem_base + child->tf->esp_user);
        for (uint32_t i = 0; i < used; i++) {
            stack_dst[i] = stack_src[i];
        }
    }
#endif

    /* TODO 5: vfork后续处理
     * - 设置子进程state=RUNNABLE
     * - 调用schedule()
     * - 返回子进程pid
     */
    child->pid = child_idx;
    child->ppid = parent->pid;
    child->state = RUNNABLE;
    child->sleep_ticks = 0;
    child->vfork_count = 0;
    schedule();
    return child->pid;
}

void do_exec(uint32_t sec, uint32_t num) {
    /* TODO 1: 恢复vfork子进程的独立内存空间
     * - 判断当前进程是否由vfork创建（mem_base != 正确基址）
     * - 若是，恢复到独立槽位
     * - 减少父进程的vfork_count
     */
    if (current->pid > 0) {
        uint32_t correct_base = 0x20000 + (current->pid - 1) * SLOT_SIZE;
        if (current->mem_base != correct_base) {
            current->mem_base = correct_base;
            if (current->ppid >= 0 && current->ppid < MAX_PCB && pcb[current->ppid].vfork_count > 0) {
                pcb[current->ppid].vfork_count--;
            }
#if VFORK_MEMORY_TEST_MODE
            if (current->ppid > 0 && current->ppid < MAX_PCB) {
                pcb[current->ppid].tf->esp_user += VFORK_STACK_OFFSET;
                if (pcb[current->ppid].state == BLOCKED) {
                    pcb[current->ppid].state = RUNNABLE;
                }
            }
#endif
        }
    }

    /* TODO 2: 从磁盘加载程序
     * - 根据传入的sec和num参数
     * - 循环调用readSect从指定扇区读取
     * - 加载到当前进程mem_base地址
     */
    for (uint32_t i = 0; i < num; i++) {
        readSect((void *)(current->mem_base + i * 512), sec + i);
    }

    /* TODO 3: 重置执行上下文
     * - eip重置为0（程序入口偏移）
     * - esp_user重置为槽位界限+1
     * - eax清零
     * - 更新GDT用户段基址
     */
    current->tf->eip = 0;
    current->tf->esp_user = SEG_LIMIT + 1;
    current->tf->eax = 0;
    update_user_seg_base(current->mem_base, current->mem_limit);
}

void do_exit() {
    /* TODO: 实现do_exit函数
     * - 将当前进程state设为UNUSED
     * - 直接触发schedule()进行调度
     * - 进程结束，槽位可被重新使用
     */
    if (current->pid > 0) {
        uint32_t correct_base = 0x20000 + (current->pid - 1) * SLOT_SIZE;
        if (current->mem_base != correct_base) {
            if (current->ppid >= 0 && current->ppid < MAX_PCB && pcb[current->ppid].vfork_count > 0) {
                pcb[current->ppid].vfork_count--;
            }
#if VFORK_MEMORY_TEST_MODE
            if (current->ppid > 0 && current->ppid < MAX_PCB) {
                pcb[current->ppid].tf->esp_user += VFORK_STACK_OFFSET;
                if (pcb[current->ppid].state == BLOCKED) {
                    pcb[current->ppid].state = RUNNABLE;
                }
            }
#endif
        }
    }

    current->state = UNUSED;
    current->sleep_ticks = 0;
    current->vfork_count = 0;
    schedule();
}

void do_sleep(int ticks) {
    /* TODO: 实现do_sleep函数
     * - 设置当前进程sleep_ticks为传入的ticks值
     * - 设置state为BLOCKED
     * - 触发schedule()让出CPU
     */
    current->sleep_ticks = ticks;
    current->state = BLOCKED;
    schedule();
}

int do_getpid() {
    return current->pid;
}

int do_getppid() {
    return current->ppid;
}
