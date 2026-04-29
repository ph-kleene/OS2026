# vfork 系统调用实现说明 (Lab 3)

本文档列出了项目中实现 `vfork` 系统调用所涉及的核心源文件及其详细功能解释。

## 1. 核心定义与配置

### `kernel/include/syscall.h`
- **角色**：系统调用号定义。
- **说明**：定义了 `SYS_VFORK (4)`，作为内核与用户空间通信的协议编号。

### `kernel/include/proc.h`
- **角色**：PCB 结构扩展与宏定义。
- **说明**：
    - `struct PCB` 增加了 `vfork_count` 成员，用于内核态的并发控制（防止父进程同时开启多个共享内存的 vfork 子进程）。
    - 定义了 `VFORK_STACK_OFFSET (32768)`，通过 32KB 的栈偏移确保父子进程在共享内存段时，栈操作不会立即相互破坏。

## 2. 内核态实现逻辑

### `kernel/core/proc.c`
- **角色**：进程控制核心。
- **说明**：
    - **`do_vfork()`**: 实现轻量级进程创建。关键点在于**不拷贝内存槽位**，而是让子进程的 `mem_base` 指向父进程的地址。同时调整子进程的 `tf->esp_user`，应用 32K 偏移。
    - **`do_exec()`**: 在进程加载新镜像时，检查其是否处于 vfork 共享状态。如果是，则将其 `mem_base` 恢复为该 PID 拥有的独立物理槽位，并释放父进程的 `vfork_count` 计数。

### `kernel/core/irq_dispatch.c`
- **角色**：中断处理与分发。
- **说明**：在 `syscallHandle` 中识别 `SYS_VFORK` 编号，将其路由至 `do_vfork()` 执行，并将结果（子进程 PID）写回 `TrapFrame` 的 `eax`。

## 3. 系统引导与应用

### `kernel/kernel.c`
- **角色**：内核初始化与 Idle 进程。
- **说明**：Idle 进程（PID 0）作为第一个运行的实体，利用内联汇编实现的 `idle_vfork()` 创建 PID 1。这是系统从内核态初始化转向执行磁盘用户程序的第一步。

### `lib/syscall.c` & `lib/myos.h`
- **角色**：用户态 API 封装。
- **说明**：
    - `myos.h` 暴露接口给用户程序。
    - `syscall.c` 通过 `int $0x80` 指令将 `vfork` 请求发送至内核，完成用户态到内核态的转换。

## 4. 测试与验证

### `app/main.c`
- **角色**：用户态测试程序。
- **说明**：通过调用 `vfork()` 并配合 `exec()` 验证内核的并发限制（连续两次 vfork 应失败）以及共享内存下的执行逻辑。
