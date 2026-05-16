# Lab4: 进程间通信与同步

本实验在 Lab3 的基础上，提供进程间通信与同步机制：1）新增父进程等待子进程结束的系统调用`wait`，完善Lab3已实现的`exit`系统调用；2）进程间通信拟利用lab3提供的vfork系统调用创建共享全局变量的父子进程，利用共享变量进行进程间数据传递，为保证共享数据访问正确，内核提供信号量与PV操作相关的系统调用：`sem_init`, `sem_destroy`, `sem_wait`, `sem_post`供用户进程在访问共享变量时正确同步；3）新增时间片轮转法进程调度算法。

## 实验要求

补全代码框架中用`TODO`标记的代码块，观察程序输出，将输出截图到实验报告内，验证你的TODO是否正确实现并对你的输出加以正确的解释，`app/main.c`中的测试包含：

- **信号量同步 (Semaphore)**：累加值精确等于预期，证明同步机制在高负载下依然可靠。
- **数据竞争复现 (Race Condition)**：最终结果显著小于预期，成功复现了因多进程竞争导致的 lost updates 现象。
- **时间片轮转 (RR Scheduling)**：通过串口日志观察到 PID 2 和 PID 3 交替打印，证明了 50ms 抢占调度的有效性。
- **Wait/Exit 完善**：测试最后打印 "Lab 4 Verification Complete"，且后续进程成功复用了 PID 槽位。

如果实现正确，你的输出会和下图类似（不包含子进程回收，请自主添加用例测试）。


```
Idle process (PID 0) started.
Child process (PID 1) starting exec...
--- Lab 4 Verification Suite (High Intensity) ---

[15:38:07] [Test 1] Semaphore Synchronization:
Final Value: 1000000 (Expected: 1000000)
RESULT: SUCCESS - Semaphore ensured data integrity.

[15:38:13] [Test 2] Race Condition (500000 per process, UNPROTECTED):
Final Value: 721232 (Expected < 1000000)
RESULT: SUCCESS - Race condition captured!

[15:38:16] [Test 3] Round-Robin Interleaving (Observe alternating PIDs):
Process 2 loop 0
Process 3 loop 0
Process 2 loop 1
Process 3 loop 1
Process 2 loop 2
Process 3 loop 2

--- Lab 4 Verification Complete ---
```

## 实验报告要求

报告中要求回答以下问题。

- 在实验的同步测试中，信号量被初始化为 1（`sem_id = sem_init(1);`）。如果把初始值改为 0 或大于 1，对临界区的保护会产生什么影响？请分别说明这两种情况会导致哪种同步问题（如死锁、竞态或过度并发）。
- `do_sem_wait` 在信号量值为 0 时将进程状态设为 BLOCKED 并调用 `schedule()`；而在值大于 0 时直接减值返回。如果把这两种情况都改为自旋忙等（如 `while (semaphores[sem_id].value == 0) ;`），对系统的调度开销和功耗会有什么变化？在什么场景下自旋等待可能更合适？
- `do_sem_wait`的实现，目前的数据结构下，插入等待队列队尾操作的时间复杂度是`O(N)`，是否可以通过修改信号量数据结构定义，将该操作的复杂度降为`O(1)`？

## 交付物

- lab4的完整代码与`[学号].pdf`。
- 压缩后压缩包的命名格式为`lab[x]-[学号]`，把`[x]`替换为当前lab的编号，`[学号]`替换为自己的学号，如`lab4-241220000.zip`。
- 具体的提交格式与提交方法见[实验主页](http://114.212.80.195:8170/oslab)

## 项目结构

本实验涉及的核心代码文件如下：

```text
.
├── kernel/
│   ├── include/
│   │   ├── proc.h          # 定义 PCB 结构（新增 time_count, next）及 Semaphore 结构
│   │   └── syscall.h       # 定义 SYS_WAIT, SYS_SEM_INIT 等新增系统调用号
│   └── core/
│       ├── proc.c          # 实现 schedule(RR), do_wait, do_exit, 信号量 PV 操作
│       └── irq_dispatch.c  # 时钟中断处理（抢占）及系统调用路由分发
├── lib/
│   ├── myos.h              # 用户态系统调用接口声明
│   └── syscall.c           # 用户态系统调用接口实现封装
└── app/
    └── main.c              # 实验验证程序（同步测试、竞态演示、RR 观察）
```

## 系统调用接口

| ID | 系统调用 | 功能描述 | 来源/变更 |
| :--- | :--- | :--- | :--- |
| 0 | `write` | 格式化输出字符串到 VGA 和串口 | Lab 2 |
| 1 | `now` | 获取 CMOS 实时时钟 | Lab 2 |
| 2 | `sleep` | 阻塞式休眠（单位：秒） | Lab 2 引入，Lab 3 重构为非忙等实现 |
| 3 | `fork` | 克隆当前进程及 128KB 物理槽位数据 | Lab 3  |
| 4 | `vfork` | 轻量级 Fork，共享内存，受并发控制 | Lab 3 |
| 5 | `exec` | 从磁盘加载应用模块覆盖当前槽位 | Lab 3 |
| 6 | `exit` | 结束当前进程并进入 ZOMBIE 态 | Lab 3 引入，Lab 4 重构 |
| 7 | `getpid` | 获取当前进程 ID | Lab 3 |
| 8 | `getppid` | 获取父进程 ID | Lab 3 |
| 9 | `wait` | 等待所有子进程结束并回收 | Lab 4 新增 |
| 10 | `sem_init` | 创建信号量并返回索引 | Lab 4 新增 |
| 11 | `sem_destroy` | 销毁指定索引的信号量 | Lab 4 新增 |
| 12 | `sem_post` | 信号量 V 操作（FIFO 唤醒） | Lab 4 新增 |
| 13 | `sem_wait` | 信号量 P 操作（FIFO 阻塞） | Lab 4 新增 |

## 核心设计与实现细节

### 1. 信号量数据结构 (SEM，10个待分配信号量)
为了高效管理阻塞进程，信号量等待队列采用**单向链表**实现，移除了原有的固定大小数组限制。

```c
struct Semaphore {
    int value;
    int used;
    struct PCB *wait_queue; // 指向等待队列队首 PCB 的指针（单向链表）
};
```
*   **sem_wait (P操作)**：若 `value > 0` 则递减；否则遍历链表将当前进程追加到 `wait_queue` 队尾（FIFO），状态设为 `BLOCKED` 并触发调度。
*   **sem_post (V操作)**：若 `wait_queue` 不为空，则直接移除链表头部的 PCB 并将其状态设为 `RUNNABLE`（O(1) 操作）；否则递增 `value`。

### 2. 进程控制块（PCB）
*   **时间片相关**：增加 `int time_count`，初始化为 5（对应 50ms）。
*   **队列管理**：增加 `struct PCB *next` 指针，用于在信号量等待队列中链接进程。
*   **进程状态定义**：
    *   `UNUSED` (0): [Lab 3] 空闲状态，PCB 槽位未被使用。
    *   `RUNNABLE` (1): [Lab 3] 就绪状态，进程在就绪队列中等待调度。
    *   `RUNNING` (2): [Lab 3] 运行状态，当前正在 CPU 上执行。
    *   `BLOCKED` (3): [Lab 3] 阻塞状态，等待事件（如锁或睡眠）而移出调度队列。
    *   `ZOMBIE` (4): [Lab 4 新增] 僵尸状态，进程已退出但尚未被父进程回收。
    *   `WAIT_CHILD` (5): [Lab 4 新增] 等待子进程状态，父进程调用 `wait` 挂起等待子进程退出。
*   **孤儿进程处理**：`do_exit` 时将所有子进程的 `ppid` 重定向至 PID 0 (Idle 进程)。

### 3. 调度算法：时间片轮转法 (Round-Robin)
*   **时钟中断**：每 10ms 触发一次，递减当前进程 `time_count`。当减至 0 且当前进程不是 Idle 进程时，强制调用 `schedule()` 进行抢占。
*   **公平扫描**：`schedule()` 从 `(current->pid + 1) % MAX_PCB` 开始循环扫描。
*   **低优先级 Idle**：优先调度 PID > 0 的就绪进程。仅当没有任何用户进程处于就绪态时，才调度 Idle 进程（PID 0）。
*   **时间片重置**：每次新进程被调度上 CPU 时，其 `time_count` 均恢复为 5 (50ms)。若进程在用完时间片前主动阻塞（如 `sleep` 或 `sem_wait`），其剩余时间片将失效，下次被调度时重新获得 50ms。

### 4. 进程回收 (Wait/Exit)
*   **do_exit**：进程将自身置为 `ZOMBIE`。检查其父进程是否正处于 `WAIT_CHILD` 状态，并且当前进程是否是该父进程最后一个活跃的子进程。若是，则唤醒父进程（设为 `RUNNABLE`）。
*   **do_wait**：
    1.  **阶段一**：扫描是否存在属于当前进程的子进程。若无，立即返回 `-1`。
    2.  **阶段二**：检查子进程是否全部为 `ZOMBIE`。若仍有活跃子进程，将自身设为 `WAIT_CHILD` 状态并挂起。
        *   *Fix*: 阻塞返回时通过 `return SYS_WAIT` 确保系统调用被正确重试。
    3.  **阶段三**：被唤醒后确认所有子进程均已结束，循环清理所有子进程 PCB 槽位（设为 `UNUSED`）并返回。

## 构建与运行

### 编译与运行
*   **清理并编译镜像**：`make clean && make os.img`
*   **运行仿真 (QEMU)**：`make run` (或手动执行 `qemu-system-i386 -drive format=raw,file=os.img -serial stdio`)

### 测试内容及验证结果
测试程序位于 `app/main.c`，涵盖了以下三个核心场景：

1.  **信号量同步 (Semaphore)**:
    *   通过 `vfork` 创建共享内存进程，使用信号量保护累加操作。
    *   **压力测试**：每个进程执行 **500000** 次 PV 操作。
    *   **验证结果**：累加值精确等于预期，证明同步机制在高负载下依然可靠。

2.  **数据竞争复现 (Race Condition)**:
    *   取消信号量保护，并发进行大量累加操作（各 500000 次）。
    *   **验证结果**：最终结果显著小于预期，成功复现了因多进程竞争导致的 lost updates 现象。

3.  **时间片轮转 (RR Scheduling)**:
    *   创建两个执行计算任务的子进程。
    *   **验证结果**：通过串口日志观察到 PID 2 和 PID 3 交替打印，证明了 50ms 抢占调度的有效性。

4.  **Wait/Exit 完善**:
    *   父进程通过 `wait()` 阻塞直到所有测试子进程变为僵尸，并成功清理。
    *   **验证结果**：测试最后打印 "Lab 4 Verification Complete"，且后续进程成功复用了 PID 槽位。


## 调试说明
*   **串口输出**：内核日志与用户态打印均同步重定向至串口 0。
*   **Bochs 验证**：建议使用 `make bochs` 验证分段保护机制对进程越界访问的拦截。
*   **QEMU 模拟速度**：为提高数据竞争现象的捕获概率，QEMU 使用 `-icount shift=8` 参数将指令执行速度降低（每条指令消耗 256 个虚拟时钟周期），使并发进程的指令交错更容易被观测到。
