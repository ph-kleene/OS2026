# Lab2 TODO 

## ① Kernel 初始化

### 1. kernel/x86/idt.c —— IDT设置（2个TODO）

```
TODO 1: 设置系统调用门
- 中断号 0x80 对应 irqSyscall
- 使用 setIntr 设置，DPL=3（允许用户态调用）

TODO 2: 设置时钟中断门
- 中断号 0x20 对应 irqTimer  
- 使用 setIntr 设置，DPL=0（仅内核可访问）
- 用于实现系统时钟和sleep功能
```

### 2. kernel/device/timer.asm —— 定时器配置（1个TODO）

```
TODO: 配置8253定时器，产生周期性中断
- 向端口0x43写入控制字（模式3，波特率发生器）
- 向端口0x40写入分频值
- 计算：输入频率1193182Hz / 目标频率100Hz = 11932
- 实现10ms产生一次时钟中断（用于sleep精度）
```

### 3. kernel/x86/gdt.c —— 切换到用户态（1个TODO）

```
TODO: 实现 enterUserSpace 函数
```

---

## ② 系统调用实现和库函数封装

### 4. kernel/x86/trap_entry.asm —— 中断入口（2个TODO）

```
TODO 1: 实现 asmDoIrq 公共中断处理入口

TODO 2: 实现 irqSyscall 系统调用入口
```

### 5. kernel/core/irq_dispatch.c —— 系统调用分发与处理（4个TODO）

```
TODO 1: 实现 syscallHandle 系统调用分发函数

TODO 2: 实现 syscallWrite 写屏功能

TODO 3: 实现 syscallNow 获取实时时钟

TODO 4: 实现 timerHandle 时钟中断处理
```

### 6. lib/doSyscall.asm —— 用户态触发系统调用（1个TODO）

```
TODO: 实现 syscall 汇编函数
- 使用栈帧传递参数
- 保存寄存器
- 触发软中断
- 恢复寄存器并返回
```

### 7. lib/syscall.c —— 高级API封装（3个TODO）

```
TODO 1: 实现 now() 函数

TODO 2: 实现 sleep() 函数

TODO 3: 实现 printf() 函数
```

---

## ③ 用户态

### 8. app/main.c —— 尝试非法操作，观察内核捕获情况

```
TODO1：在代码里添加地址访问越界的操作，使用make bochs运行项目，观察内核捕获异常的情况

TODO2: 在代码里添加特权指令，使用make run运行项目，观察内核捕获异常的情况。

注意：两个TODO需要分开测试，TODO1用 make bochs，TODO2用 make run，提交代码时请保留你添加的代码。
```

---

## TODO 汇总表

| 分类               | 文件                       | 知识点                             |
| ------------------ | -------------------------- | ---------------------------------- |
| **① Kernel初始化** | kernel/x86/idt.c           | IDT、系统调用门                    |
|                    | kernel/device/timer.asm    | 8253定时器、中断频率               |
|                    | kernel/x86/gdt.c           | IRET、Ring切换                     |
| **② 系统调用**     | kernel/x86/trap_entry.asm  | 中断入口、栈帧                     |
|                    | kernel/core/irq_dispatch.c | 分发、处理、地址转换               |
|                    | lib/doSyscall.asm          | int 0x80                           |
|                    | lib/syscall.c              | API封装                            |
| **③ 用户态**       | app/main.c                 | 地址访问越界及特权指令所产生的异常 |

