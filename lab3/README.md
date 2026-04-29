# Lab3: 进程管理与调度

本实验在 Lab2 的基础上，将系统从单进程环境扩展为支持**多进程并发管理与抢占式调度**的操作系统原型。实验的核心目标是在保持“纯分段模式”内存隔离的前提下，实现进程控制块（PCB）、动态 GDT 切换机制以及经典的进程控制系统调用。

lab3 框架代码：lab3.zip[待上传]

## 实验要求

补全代码框架中用`TODO`标记的代码块，观察程序输出，将输出截图到实验报告内，验证你的TODO是否正确实现并对你的输出加以正确的解释，包括：

- **fork槽位上限**：循环fork 6次，前3-4次成功创建子进程，后续返回-1（`No more slots available`）
- **子进程独立内存**：fork创建的子进程修改全局变量后，父进程不受影响
- **vfork二次创建子进程限制**：父进程vfork创建子进程后，在子进程调用exec前再次vfork，应返回-1（`Concurrency Control Works!`）
- **vfork共享内存**：vfork创建的子进程修改全局变量后，父进程看到的是修改后的值
- **调度正常切换**：子进程sleep期间，系统能正常切换到其他进程执行

由于每个人的输出有所不同，本次实验没有严格的“正确答案”，正确实现的输出可能与下面类似（下列输出不包含对fork和vfork的父子进程全局变量是否共享的验证）：

<img src="C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20260331152707408.png" alt="image-20260331152707408" style="zoom:50%;" />

## 实验报告要求

报告中要求回答以下问题。

- fork与vfork的本质区别是什么？在内存管理上有何不同？
- vfork为何需要限制其创建的子进程数量？它如何通过与exec的配合来控制子进程数量？
- 描述schedule()函数中上下文切换的过程。

## 交付物

- lab3的完整代码与`[学号].pdf`。
- 压缩后压缩包的命名格式为`lab[x]-[学号]`，把`[x]`替换为当前lab的编号，`[学号]`替换为自己的学号，如`lab3-241220000.zip`。
- 具体的提交格式与提交方法见[实验主页](http://114.212.80.195:8170/oslab)

## 项目结构

![image-20260331152835369](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20260331152835369.png)

## 核心设计与实现细节

### 1. 内存与进程布局 (5-Slot 模型)

本实验采用**固定槽位（Fixed Slots）**分配方案，内核逻辑上控制着从 `0` 到 `1MB` 的完整物理内存空间。通过在调度时动态修改 GDT 中用户段的基地址（Base），实现了各进程在逻辑上均从地址 0 开始运行，但在物理上被隔离在不同的 128KB 槽位中。

| 区域 | 物理地址范围 | 大小 | 说明 |
| :--- | :--- | :--- | :--- |
| **内核全局控制区** | `0x00000000 - 0x000FFFFF` | 1 MB | 内核掌握的完整物理空间，包含所有进程槽位与硬件映射 |
| ↳ **Slot 0 (Idle)** | `0x00000000 - 0x0001FFFF` | 128 KB | 包含 BIOS 数据、内核镜像（加载于 0x10000）、Idle 进程 |
| ↳ **Slot 1** | `0x00020000 - 0x0003FFFF` | 128 KB | 用户进程 1 物理空间 |
| ↳ **Slot 2** | `0x00040000 - 0x0005FFFF` | 128 KB | 用户进程 2 物理空间 |
| ↳ **Slot 3** | `0x00060000 - 0x0007FFFF` | 128 KB | 用户进程 3 物理空间 |
| ↳ **Slot 4** | `0x00080000 - 0x0009FFFF` | 128 KB | 用户进程 4 物理空间 |
| ↳ **内核初始栈** | `0x00000000 - 0x0000FFFF` | 64 KB | 内核初始化期间使用的栈（栈顶 0x10000） |
| ↳ **VGA 显存映射** | `0x000B8000 - 0x000BFFFF` | 32 KB | 硬件 VGA 文本模式显存输出区 |

*   **Kernel Reload**：内核加载地址调整为 `0x10000`。由于内核完整位于 Slot 0 内部，Idle 进程（PID 0）的执行逻辑实体是内核代码的一部分，被构造为第一个用户态运行的进程，再由Idle进程vfork并exec加载执行磁盘上的app程序。
*   **fork**：传统的进程克隆机制。执行完整的 128KB 物理槽位拷贝（代码、数据、栈）。子进程获得完全独立的物理内存空间。
*   **vfork**：轻量级进程创建机制。在实验中，不仅用于 Idle 进程启动第一个用户程序，也已开放给用户进程使用。`vfork` 创建的子进程与父进程共享内存空间（代码、数据、堆栈），仅通过栈指针偏移（-32KB）避免简单的栈冲突。同一时间内，父进程仅允许有一个尚未执行 `exec` 的 `vfork` 子进程。设计意图是配合 `exec` 使用，在执行 `exec` 时，系统会解除内存共享，恢复子进程独立的内存槽位并加载新程序，同时减少父进程的 `vfork` 计数。
*   **段隔离**：每个槽位通过 GDT 的 `SEG_UCODE` 和 `SEG_UDATA` 进行重定位。

### 2. 进程控制块 (PCB)
每个进程由 `struct PCB` 管理，包含以下核心分量：
*   **私有内核栈 (`kstack`)**：4KB 独立空间，支持中断嵌套。
*   **上下文指针 (`tf`)**：指向保存在内核栈上的寄存器现场。
*   **状态机**：支持 `UNUSED`, `RUNNABLE`, `RUNNING`, `BLOCKED`。
*   **休眠计数器 (`sleep_ticks`)**：实现非忙等 `sleep` 的关键。
*   **vfork 计数器 (`vfork_count`)**：记录当前进程已发起的活动 `vfork` 次数，用于子进程创建控制。

### 3. 调度算法 (Tail-to-Head Priority)
调度器 `schedule()` 采用从 `pcb[4]` 到 `pcb[0]` 的**逆向扫描策略**：
1.  **统一扫描**：统一从高 PID (Slot 4) 向低 PID (Slot 0) 扫描，查找 `RUNNABLE` 或 `RUNNING` 状态的进程。
2.  **优先级保障**：此策略确保高优先级（高 PID）进程即便在运行中 (`RUNNING`) 也不会被低优先级就绪进程抢占，同时 Idle 进程 (PID 0) 作为最低优先级的保底。
3.  **抢占触发**：时钟中断（100Hz）负责递减 `sleep_ticks`，唤醒进程，并强制触发调度。

### 4. 系统调用接口

| ID | 系统调用 | 功能描述 | 来源/变更 |
| :--- | :--- | :--- | :--- |
| 0 | `write` | 格式化输出字符串到 VGA 和串口 | Lab 2 基础功能 |
| 1 | `now` | 获取 CMOS 实时时钟 | Lab 2 基础功能 |
| 2 | `sleep` | 阻塞式休眠（单位：秒） | Lab 2 引入，Lab 3 重构为非忙等实现 |
| 3 | `fork` | 克隆当前进程及 128KB 物理槽位数据 | Lab 3 新增 |
| 4 | `vfork` | 轻量级 Fork，共享内存，受控制 | Lab 3 新增 |
| 5 | `exec` | 从磁盘加载应用模块覆盖当前槽位 | Lab 3 新增 |
| 6 | `exit` | 结束当前进程并释放槽位 | Lab 3 新增 |
| 7 | `getpid` | 获取当前进程 ID | Lab 3 新增 |
| 8 | `getppid` | 获取父进程 ID | Lab 3 新增 |

#### 系统调用实现细节 (Lab 3)

*   **`fork`**: 完整拷贝父进程的 128KB 物理内存槽位到新分配的槽位。内核会复制 PCB 及内核栈，并调整子进程 `eax` 为 0 以区分父子进程。
*   **`vfork`**: 与 `fork` 不同，子进程与父进程暂时共享物理内存和代码段。为避免冲突，子进程栈顶通过 `VFORK_STACK_OFFSET` (32K) 进行偏移。系统限制每个父进程同时只能有一个活动（未 `exec`）的 `vfork` 子进程，以确保共享状态的安全性。
*   **`exec`**: 负责从磁盘特定扇区加载程序镜像。若当前进程为 `vfork` 创建，`exec` 会解除共享状态，将其重定位回原本独立的物理槽位，并减少父进程的 `vfork_count` 计数。
*   **`sleep`**: 弃用了 Lab 2 的忙等待，改为修改进程状态为 `BLOCKED` 并设置 `sleep_ticks`。由时钟中断（`irq_dispatch.c`）负责每秒 100 次的计数递减，并在清零时将进程置回 `RUNNABLE`。
*   **`exit`**: 将当前进程状态置为 `UNUSED` 并立即触发 `schedule()` 调度。被释放的槽位可供后续 `fork` 或 `vfork` 重新使用。
*   **`getpid / getppid`**: 直接从当前运行进程的 `struct PCB` 结构中提取 `pid` 或 `ppid` 字段返回。

## 磁盘镜像结构
`os.img` 的布局如下，支持最大 64KB 的内核与应用模块：

| 扇区范围 | 内容 | 大小 | 加载/映射位置 |
| :--- | :--- | :--- | :--- |
| 0 | Bootloader | 512 B | `0x7C00` |
| 1 - 128 | Kernel (kernel.bin) | 64 KB | `0x10000` |
| 129 - 256 | App (app.bin) | 64 KB | 根据 `exec` 加载到当前 Slot |

## 构建与运行

### 测试内容及测试通过展示

系统启动后，Idle 进程（PID 0）通过 `vfork` + `exec` 启动用户主应用（PID 1）。`main.c`包含以下自动化测试：

1.  **Fork 槽位上限测试**：PID 1 尝试连续创建子进程。由于系统限制 `MAX_PCB = 5`，当 Slot 1-4 被占用后，后续 `fork` 应返回 `-1` 并报错。
2.  **并发执行与非忙等 Sleep**：成功创建的子进程（PID 2, 3, 4）进入 10 秒休眠。系统应通过时钟中断在多个就绪进程间自动切换，并准确处理 `BLOCKED` 状态。
3.  **vfork 二次创建子进程限制**：父进程发起 `vfork` 创建子进程 1。在子进程 1 调用 `exec` 之前，父进程再次尝试 `vfork` 应被内核拒绝（返回 `-1`），验证共享内存环境下的安全保护。
4.  **exec 槽位重定位**：`vfork` 子进程调用 `exec` 后，内核将其内存基地址恢复到独立的物理槽位，并释放父进程的 `vfork` 计数。
5.  如果有余力，你可以自己修改`main.c`做一些你感兴趣的测试。

此外，按照我们的TODO SUMMARY，你还应该在`app/main.c`中增加代码，分别验证父子进程是否会共享全局变量。

**测试成功时的终端可能输出：**

```text
User App Started. My PID: 1, PPID: 0
Testing Fork Failure (exhausting slots)...
Parent created child with PID 2
Parent created child with PID 3
Parent created child with PID 4
Fork failed as expected! (No more slots available)
...
Testing vfork concurrency control...
vfork Child 1 (PID 5) created, sleeping before exec...
Parent: Created vfork child 1 (PID 5), attempting vfork 2...
Parent: vfork 2 failed as expected (Concurrency Control Works!)
Parent PID 1 going into main loop.
...
vfork Child 1 calls exec to reload app...
(User App restarts with PID 5)
```

## 调试说明
*   **串口输出**：内核日志与用户态打印均同步重定向至串口 0。
*   **Bochs 验证**：如果`bochs`可以正常使用，可以尝试使用 `make bochs` 验证分段保护机制对进程越界访问的拦截。
