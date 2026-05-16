

## Lab4 TODO SUMMARY

## ①wait/exit 系统调用完善

### 1. kernel/core/proc.c —— exit实现完善

```
TODO: 实现完整的 exit 功能，处理两种情况
- 情况1：父进程正在等待（state == WAIT_CHILD）
  → 将当前进程设为 ZOMBIE，等待父进程调用 wait 回收
- 情况2：父进程不存在或不在等待
  → 将当前进程设为 UNUSED，自己释放 PCB 槽位

实现步骤：
1. 将当前进程 state 设为 ZOMBIE
2. 遍历所有子进程，将它们的 ppid 设为 0（孤儿进程交给 idle 收养）
3. 检查父进程是否在 WAIT_CHILD 状态
4. 若父进程在等待且当前进程是最后一个活跃子进程，唤醒父进程
5. vfork 相关内存恢复（如有）
6. 检查父进程是否存在且在等待子进程
   - 若父进程不存在（ppid < 0）或父进程状态不是 WAIT_CHILD
   - 将当前进程 state 改为 UNUSED（自行回收）
7. 调用 schedule()


```

### 2. kernel/core/proc.c —— wait实现

```
TODO 1: 子进程存在性检查
- 遍历pcb数组，查找ppid等于当前pid的子进程
- 若无子进程，立即返回-1

TODO 2: 活跃子进程检查
- 检查子进程中是否存在状态不为ZOMBIE的进程
- 若有活跃子进程，则将当前进程state设为WAIT_CHILD
- 实现Mesa重试机制：current->tf->eip -= 2
- 调用schedule()让出CPU
- 返回SYS_WAIT系统调用号

TODO 3: 僵尸进程清理
- 若所有子进程均为ZOMBIE状态
- 遍历并清理所有僵尸子进程（state设为UNUSED）
- 返回0表示成功回收
```

---

## ② 信号量PV操作实现

### 3. kernel/core/proc.c —— 信号量初始化与销毁

```
TODO 1: sem_init实现
- 遍历semaphores数组查找空闲信号量
- 设置used标志为1
- 设置value为传入的初始值
- 设置wait_queue为NULL
- 返回信号量索引

TODO 2: sem_destroy实现
- 检查sem_id有效性
- 若信号量已使用，则设置used为0
- 清理wait_queue为NULL
- 返回0或错误码
```

### 4. kernel/core/proc.c —— sem_wait(P操作)实现

```
TODO 1: 参数校验
- 检查sem_id有效性（0~MAX_SEM-1）
- 检查信号量是否已被初始化(used==1)

TODO 2: 资源检查与获取
- 若semaphores[sem_id].value > 0
- 递减value并返回0

TODO 3: 阻塞与入队
- 若value == 0
- 将当前进程加入wait_queue等待队列（FIFO）
- 设置当前进程state为BLOCKED
- 实现Mesa重试：current->tf->eip -= 2
- 调用schedule()让出CPU
- 返回SYS_SEM_WAIT
```

### 5. kernel/core/proc.c —— sem_post(V操作)实现

```
TODO 1: 参数校验
- 检查sem_id有效性
- 检查信号量是否已使用

TODO 2: 值递增
- 无条件递增semaphores[sem_id].value

TODO 3: 唤醒等待进程
- 检查wait_queue是否不为NULL
- 从队列头部移除第一个等待进程
- 将被唤醒进程state设为RUNNABLE
- 将next指针设为NULL使其脱离队列
- 注意：唤醒后不立即调用schedule()，减少上下文切换开销
```

---

## ③ 时间片轮转调度实现

### 6. kernel/core/proc.c —— schedule函数重构

```
TODO 1: 扫描策略改为时间片轮转
- 起始扫描位置设为(current->pid + 1) % MAX_PCB
- 循环扫描pcb数组，查找RUNNABLE或RUNNING状态的进程

TODO 2: Idle进程低优先级处理
- 优先调度用户进程(PID > 0)
- 仅当无用户进程就绪时才调度Idle进程(PID 0)

TODO 3: 上下文切换与时间片重置
- 若有进程切换：
  - 将当前进程state从RUNNING改为RUNNABLE
  - 设置next进程state为RUNNING
  - 重置next进程time_count为5（50ms）
  - 更新current指针
  - 更新TSS.esp0指向新进程内核栈顶
  - 调用update_user_seg_base切换GDT基址
```

---

## ④ 时钟中断与抢占调度

### 7. kernel/core/irq_dispatch.c —— timerHandle完善

```
TODO 1: 时间片递减与抢占
- 当前进程为RUNNING状态时递减其time_count
- 当time_count <= 0时，调用schedule()进行抢占

TODO 2: 调度触发时机补充
- 即使当前进程不在RUNNING状态（如刚唤醒），也调用schedule()
```

---

## ⑤ 系统调用分发扩展

### 8.kernel/core/irq_dispatch.c —— 新增系统调用分发

```
TODO 1: 实现syscallWait
- 调用do_wait()
- 将返回值写入tf->eax

TODO 2: 实现syscallSemInit  
- 从tf->ebx获取value参数
- 调用do_sem_init()
- 将返回值写入tf->eax

TODO 3: 实现syscallSemDestroy
- 从tf->ebx获取sem_id参数
- 调用do_sem_destroy()
- 将返回值写入tf->eax

TODO 4: 实现syscallSemPost
- 从tf->ebx获取sem_id参数
- 调用do_sem_post()
- 将返回值写入tf->eax

TODO 5: 实现syscallSemWait
- 从tf->ebx获取sem_id参数
- 调用do_sem_wait()
- 将返回值写入tf->eax
```

---

## ⑥ 用户态接口

### 9. lib/syscall.c —— 新增系统调用封装

```
TODO 1: 实现wait用户接口
- 调用syscall(SYS_WAIT, ...)

TODO 2: 实现sem_init用户接口
- 传入value参数调用syscall(SYS_SEM_INIT, ...)

TODO 3: 实现sem_destroy用户接口
- 传入sem_id参数调用syscall(SYS_SEM_DESTROY, ...)

TODO 4: 实现sem_post用户接口
- 传入sem_id参数调用syscall(SYS_SEM_POST, ...)

TODO 5: 实现sem_wait用户接口
- 传入sem_id参数调用syscall(SYS_SEM_WAIT, ...)
```

## ⑦ 测试用例新增

### 10. app/main.c ——新增父进程未调用wait时，子进程能否回收

```
TODO: 测试父进程未调用 wait 时，子进程 exit 后能被正确回收，后续还能再创建新的进程
```

---

## TODO汇总表

| 分类                   | 文件                       | 知识点                                           |
| ---------------------- | -------------------------- | ------------------------------------------------ |
| **① wait/exit完善**    | kernel/core/proc.c         | 僵尸进程、孤儿进程处理、父进程唤醒机制           |
| **② 信号量PV操作**     | kernel/core/proc.c         | 信号量初始化、P操作阻塞、V操作唤醒、Mesa重试     |
| **③ 时间片轮转调度**   | kernel/core/proc.c         | RR扫描策略、时间片管理、抢占式调度               |
| **④ 时钟中断完善**     | kernel/core/irq_dispatch.c | 时间片递减、抢占触发、调度时机补充               |
| **⑤ 系统调用分发**     | kernel/core/irq_dispatch.c | wait/sem_init/sem_destroy/sem_post/sem_wait分发  |
| **⑥ 用户态接口与测试** | lib/syscall.c              | 新增系统调用用户接口封装                         |
| **⑦子进程回收测试**    | app/main.c                 | 父进程未调用 wait 时，子进程 exit 后能被正确回收 |

---

