# sem_wait 系统调用实现说明 (Lab 4)

本文档详细描述了 `sem_wait` 系统调用（P 操作）在内核中的实现细节。

## 1. 核心定义与配置

### `kernel/include/syscall.h`
- **角色**：系统调用号定义。
- **说明**：定义了 `SYS_SEM_WAIT (13)`。这是用户态程序请求获取信号量资源的唯一标识。

### `kernel/include/proc.h`
- **角色**：信号量数据结构定义。
- **说明**：
    - 定义了 `struct Semaphore`，包含计数值 `value`、使用标志 `used` 和等待队列 `wait_queue`（PCB 链表）。

## 2. 内核态实现逻辑

### `kernel/core/proc.c`
- **角色**：同步原语实现。
- **说明**：
    - **`do_sem_wait(int sem_id)`**:
        1. **参数校验**：检查 `sem_id` 是否在有效范围内（0 ~ MAX_SEM-1）且该信号量已被初始化。若校验失败，返回 -1。
        2. **资源检查**：判断 `semaphores[sem_id].value` 是否大于 0。
        3. **成功获取资源**：若 `value > 0`，则将 `value` 减 1，并立即返回 0，进程继续执行。
        4. **资源不可用（阻塞逻辑）**：
            - 将当前进程 `current` 追加到该信号量的 `wait_queue` 链表末尾。
            - 将当前进程状态 `current->state` 修改为 `BLOCKED`。
            - **Mesa 重试机制**：将陷阱帧中的 `current->tf->eip` 减去 2（回退到用户态 `int $0x80` 指令的位置）。
            - **主动让出**：调用 `schedule()` 切换到其他进程。
            - **返回值设置**：返回 `SYS_SEM_WAIT`。这样当进程被唤醒并重新获得 CPU 后，内核栈的 `eax` 将保持为系统调用号，配合回退的 `eip` 立即触发重试逻辑。

## 3. 中断处理与原子性保护

### `kernel/x86/idt.c`
- **说明**：由于系统调用通过 **Interrupt Gate (0x80)** 进入，硬件会自动关闭 `EFLAGS.IF` 位。这保证了 `do_sem_wait` 内部从“检查资源”到“设置阻塞状态并调度”的整个过程是绝对原子的，不会受到时钟中断干扰。

## 4. 用户态接口

### `lib/syscall.c`
- **角色**：系统调用封装。
- **说明**：`int sem_wait(int sem_id)` 通过汇编指令将 `sem_id` 放入 `ebx` 寄存器，并触发 `int $0x80`。
