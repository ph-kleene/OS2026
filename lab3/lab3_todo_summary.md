# Lab3 TODO SUMMARY

## ① 进程控制块(PCB)设计与初始化

### 1. kernel/kernel.c —— 进程初始化

```
TODO 1: PCB初始化循环
- 遍历MAX_PCB个进程控制块
- 初始化各字段默认值
- 为不同槽位设置mem_base（Slot 0基址0，Slot 1基址0x20000...）

TODO 2: 构造Idle进程的初始TrapFrame
- 设置cs/ds/es/fs/gs/ss_user等段寄存器
- 设置esp_user为用户栈顶
- 设置eip指向idle_main函数入口
- 设置eflags（IF=1允许中断）
- 将TrapFrame放置在内核栈顶

TODO 3: 完成从内核态切换到用户态的iret序列
- 加载TrapFrame到esp
- popal恢复通用寄存器
- pop段寄存器
- iret切换到用户态
```

---

## ② 进程创建：fork与vfork

### 2. kernel/core/proc.c —— fork实现

```
TODO 1: 查找空闲PCB槽位
- 遍历pcb[1]到pcb[MAX_PCB-1]
- 找到第一个state==UNUSED的进程
- 若无空闲则返回-1

TODO 2: 复制128KB物理内存槽位
- 使用SLOT_SIZE循环拷贝
- 从父进程mem_base复制到子进程mem_base

TODO 3: 复制内核栈和TrapFrame
- 拷贝KSTACK_SIZE字节的内核栈
- 重新计算子进程tf相对于kstack的偏移

TODO 4: 调整子进程TrapFrame
- 设置子进程eax=0（fork返回值：子进程返回0）
- 设置pid和ppid
- 设置state=RUNNABLE
- 设置vfork_count=0

TODO 5: 触发调度并返回
- 调用schedule()
- 返回子进程pid
```

### 3. kernel/core/proc.c —— vfork实现

```
TODO 1: 检查vfork并发控制
- 父进程vfork_count++
- 若vfork_count>1则失败回退，返回-1
- 确保同一时间父进程只有一个未exec的vfork子进程

TODO 2: 设置vfork特殊属性——共享内存
- 子进程mem_base临时指向父进程mem_base（区别于fork的独立拷贝）
- 子进程栈顶偏移VFORK_STACK_OFFSET(32KB)避免栈冲突

TODO 3: 复制TrapFrame（与fork类似但内存共享）
- 拷贝内核栈
- 调整子进程esp_user
- 设置子进程eax=0

TODO 4: vfork后续处理
- 设置子进程state=RUNNABLE
- 调用schedule()
- 返回子进程pid
```

---

## ③ 进程执行与退出

### 4. kernel/core/proc.c —— exec实现

```
TODO 1: 恢复vfork子进程的独立内存空间
- 判断当前进程是否由vfork创建（mem_base != 正确基址）
- 若是，恢复到独立槽位
- 减少父进程的vfork_count

TODO 2: 从磁盘加载程序
- 根据传入的sec和num参数
- 循环调用readSect从指定扇区读取
- 加载到当前进程mem_base地址

TODO 3: 重置执行上下文
- eip重置为0（程序入口偏移）
- esp_user重置为槽位界限+1
- eax清零
- 更新GDT用户段基址
```

### 5. kernel/core/proc.c —— exit实现

```
TODO: 实现do_exit函数
- 将当前进程state设为UNUSED
- 直接触发schedule()进行调度
- 进程结束，槽位可被重新使用
```

### 6. kernel/core/proc.c —— sleep实现

```
TODO: 实现do_sleep函数
- 设置当前进程sleep_ticks为传入的ticks值
- 设置state为BLOCKED
- 触发schedule()让出CPU
```

---

## ④ 调度算法

### 7. kernel/core/proc.c —— 调度器实现

```
TODO 1: 实现schedule()函数——逆向扫描策略
- 从pcb[MAX_PCB-1]向pcb[0]扫描
- 查找第一个RUNNABLE或RUNNING状态的进程
- 实现高PID优先的调度策略

TODO 2: 上下文切换核心逻辑
- 若有进程切换：
  - 将当前进程state从RUNNING改为RUNNABLE
  - 设置next进程state为RUNNING
  - 更新current指针
  - 更新TSS.esp0指向新进程内核栈顶
  - 调用update_user_seg_base切换GDT基址
```

---

## ⑤ 中断处理与系统调用分发

### 8. kernel/core/irq_dispatch.c —— 时钟中断与进程唤醒

```
TODO 1: 实现timerHandle——递减休眠计数器
- ticks计数器++
- 遍历所有PCB
- 若进程state==BLOCKED且sleep_ticks>0，则递减
- 若sleep_ticks减至0，则state改为RUNNABLE
- 调用schedule()进行调度
```

### 9. kernel/core/irq_dispatch.c —— 进程相关系统调用分发

```
TODO 1: 实现syscallFork
- 调用do_fork()
- 将返回值写入tf->eax

TODO 2: 实现syscallVfork  
- 调用do_vfork()
- 将返回值写入tf->eax

TODO 3: 实现syscallExec
- 从tf->ebx/tf->ecx获取参数(sec, num)
- 调用do_exec()

TODO 4: 实现syscallExit
- 调用do_exit()

TODO 5: 实现syscallSleep
- 从tf->ebx获取ticks参数
- 调用do_sleep()

TODO 6: 实现syscallGetPid/syscallGetPpid
- 调用do_getpid()/do_getppid()
- 将返回值写入tf->eax
```

---

## ⑥ GDT动态切换

### 10. kernel/x86/gdt.c —— 用户段基址动态更新

```
TODO: 实现update_user_seg_base函数
- 根据传入的base和limit参数
- 重新构造gdt[SEG_UCODE]段描述符
- 重新构造gdt[SEG_UDATA]段描述符
- 实现不同进程槽位的内存隔离与重定位
```

---

## ⑦ 用户态测试

### 11. app/main.c —— 进程管理与调度测试

```
TODO: 添加全局变量用于测试fork/vfork的内存共享特性
- 定义一个全局变量（如计数器）
- 在fork/vfork后，父子进程分别修改该变量
- 通过输出观察值的变化，验证fork为独立内存、vfork为共享内存
- 在main函数末尾的while循环之前添加上述测试代码
- 先用fork测试，观察父子进程修改全局变量是否互不影响
- 再用vfork测试，观察父子进程修改全局变量是否互相影响
```

---

## TODO汇总表

| 分类                  | 文件                       | 知识点                                |
| --------------------- | -------------------------- | ------------------------------------- |
| **① PCB设计与初始化** | kernel/kernel.c            | TrapFrame构造、iret切换、进程初始化   |
| **② fork/vfork**      | kernel/core/proc.c         | fork内存复制、vfork共享内存、并发控制 |
| **③ exec/exit/sleep** | kernel/core/proc.c         | 程序加载、进程退出、阻塞式休眠        |
| **④ 调度算法**        | kernel/core/proc.c         | 逆向扫描、高优先级调度、上下文切换    |
| **⑤ 中断与系统调用**  | kernel/core/irq_dispatch.c | 时钟中断、sleep唤醒、系统调用分发     |
| **⑥ GDT动态切换**     | kernel/x86/gdt.c           | 段基址重定位、内存隔离                |
| **⑦ 用户态测试**      | app/main.c                 | fork/vfork内存共享特性验证            |

---

