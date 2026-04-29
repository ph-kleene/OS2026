# sleep 系统调用实现说明 (Lab 3)

本文档列出了项目中实现 `sleep` 系统调用所涉及的核心源文件及其详细功能解释。

## 1. 核心定义与配置

### `kernel/include/syscall.h`
- **角色**：系统调用号定义。
- **说明**：定义了 `SYS_SLEEP (2)`。这是用户程序请求阻塞休眠的指令编号。

### `kernel/include/proc.h`
- **角色**：PCB 结构扩展与函数声明。
- **说明**：
    - 在 `struct PCB` 中增加了 `int sleep_ticks;` 字段，用于记录进程剩余的休眠时间（以时钟节拍为单位）。
    - 声明了内核处理函数 `void do_sleep(int ticks);`。

## 2. 内核态实现逻辑

### `kernel/core/proc.c`
- **角色**：休眠状态切换。
- **说明**：
    - **`do_sleep()`**: 实现了非忙等的休眠触发。
        1. **设置计数**：将当前进程的 `sleep_ticks` 设置为传入的参数值。
        2. **阻塞进程**：将进程状态修改为 `BLOCKED`。
        3. **触发调度**：立即调用 `schedule()` 切换到其他 `RUNNABLE` 进程，从而释放 CPU 资源，避免 Lab 2 中的忙等待。

### `kernel/core/irq_dispatch.c`
- **角色**：休眠管理与唤醒中心。
- **说明**：
    - **系统调用分发**：在 `syscallHandle` 中拦截 `SYS_SLEEP`，并将参数（休眠节拍数）从 `tf->ebx` 传递给 `do_sleep`。
    - **`timerHandle` (唤醒逻辑)**：时钟中断（100Hz）的处理函数。
        1. **递减计数**：每隔 10ms 遍历所有 PCB，对处于 `BLOCKED` 状态且 `sleep_ticks > 0` 的进程执行自减。
        2. **解除阻塞**：当 `sleep_ticks` 递减至 0 时，将进程状态修改回 `RUNNABLE`。
        3. **重新调度**：在中断结束前调用 `schedule()`，确保被唤醒的进程有机会立即获得执行。

## 3. 用户态接口与库实现

### `lib/syscall.c` & `lib/myos.h`
- **角色**：用户态封装。
- **说明**：
    - `myos.h` 声明了 `void sleep(int second);`。
    - `syscall.c` 实现了具体封装：
        - **单位转换**：由于内核以 ticks（1/100秒）为单位，而 API 以秒为单位，封装函数会将 `second * 100` 后传递给内核。
        - **中断触发**：通过 `int $0x80` 陷入内核。

## 4. 应用与演示

### `app/main.c`
- **角色**：逻辑验证。
- **说明**：
    - 在多进程并发场景下，子进程通过 `sleep(10)` 模拟耗时操作，这验证了系统的并发调度能力。
    - 在 `fork` 和 `vfork` 测试中，父进程利用 `sleep` 等待子进程执行完毕或完成 `exec` 替换，展示了休眠在进程同步中的基础作用。
