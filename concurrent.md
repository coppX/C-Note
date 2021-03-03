## 原子操作
原子操作表示不会被线程调度(那么进程调度呢？)打断的操作，操作从开始到结束期间不会有其他的线程切换，把多个操作步骤看作一个不可分割的整体。
原子操作有两个属性
- 由于原子操作是不可分的，因此，来自不同线程的同意对象的第二个原子操作只能在第一个原子操作之前或之后获取该对象的状态。
- 基于其memory_order参数，原子操作可以针对同一个线程中其他原子操作的影响可见性建立排序要求。因此，它会抑制违反排序要求的编译器优化。  
使用原子操作可以不需要使用互斥锁就能做好数据同步。
## 原子类型
标准原子类型定义在`<atomic>`中，这些类型的操作都是原子的。
| 原子类型            |  相关特化类                          |
| -------------------| ---------------------              |
|`atomic_bool`       |`std::atomic<bool>`                 |
|`atomic_char`       |`std::atomic<char>`                 |
|`atomic_schar`      |`std::atomic<signed char>`          |
|`atomic_uchar`      |`std::atomic<unsigned char>`        |
|`atomic_int`        |`std::atomic<int>`                  |
|`atomic_uint`       |`std::atomic<unsigned>`             |
|`atomic_short`      |`std::atomic<short>`                |
|`atomic_ushort`     |`std::atomic<unsigned short>`       |
|`atomic_long`       |`std::atomic<long>`                 |
|`atomic_ulong`      |`std::atomic<unsigned long>`        |
|`atomic_llong`      |`std::atomic<long long>`            |
|`atomic_ullong`     |`std::atomic<unsigned long long>`   |
|`atomic_char16_t`   |`std::atomic<char16_t>`             |
|`atomic_char32_t`   |`std::atomic<char32_t>`             |
|`atomic_wchar_t`    |`std::atomic<wchar_t>`              |

表格左边是原子类型，右边表示的是用atomic<T>模板进行特化的，不光可以使用内置类型进行特化，也可以使用自定义类型进行特化。  

标准原子类型不能进行拷贝和赋值，它们没有拷贝构造函数和拷贝赋值运算符。标准原子类型还具有`is_lock_free()`成员函数(`std::atomic_flag`除外)，如果返回`true`表示这个原子类型内部是用原子指令来实现的原子操作，返回`false`表示原子内部是用的互斥锁来模拟的原子操作，具体实现跟类型和编译的目标硬件有关。原子类型支持`load()`、`store()`、`exchange()`、`compare_exchange_weak()`、`compare_exchange_strong()`、`fetch_add()`、`fetch_sub()`、`fetch_and()`、`fetch_or()`、`fetch_xor()`操作
### 原子类型上的操作
操作分为三类,每种操作都可以选择一个内存序参数，用来指定存储的顺序(如果不选就默认是顺序一致性`memory_order_seq_cst`)
1. store操作，可选内存序:
    - `memory_order_relaxed`
    - `memory_order_release`
    - `memory_order_seq_cst`
2. load操作，可选内存序:
    - `memory_order_relaxed`
    - `memory_order_consume`
    - `memory_order_acquire`
    - `memory_order_seq_cst`
3.  read-modify-write(读-改-写)操作，可选内存序:
    - `memory_order_relaxed`
    - `memory_order_consume`
    - `memory_order_acquire`
    - `memory_order_release`
    - `memory_order_acq_rel`
    - `memory_order_seq_cst`
## 原子操作的内存序
这里有六个内存序列选项可应用于对原子类型的操作:
1. `memory_order_relaxed`
2. `memory_order_consume`
3. `memory_order_acquire`
4. `memory_order_release`
5. `memory_order_acq_rel`
6. `memory_order_seq_cst`
   
除非为特定操作指定一个序列选项，要不然内存序列默认都是`memory_order_seq_cst`。这六种选项代表了三种内存模型：顺序一致性(sequentially consistent)，获取-释放序(memory_order_consume,memory_order_acquire,memory_order_release和memory_order_acq_rel)和自由序(memory_order_relaxed)。不同的内存序在不同的CPU架构下功耗不同，这些内存序将影响一个线程内部的分配如何在另一个线程内变得可见。
### 顺序一致性
memory_order_seq_cst有此内存顺序的加载操作进行获得操作，存储操作进行释放操作，而读修改写操作进行获得操作和释放操作，再加上存在一个单独全序，其中所有线程以同一顺序观测到所有修改。程序中的行为从任意角度去看，序列都保持一定顺序。如果原子实例的所有操作都是序列一致的，那么多线程就会如单线程那样以某种特殊的排序执行。从同步角度看，是对同一变量的存储和加载操作的同步。这就提供了一种对两个(以上)线程操作的排序约束，但顺序一致的功能要比排序约束大的多，所以对于使用顺序一致的原子操作，都会存储值后再加载。
### 获取-释放序
### 自由序
## std::thread

## std::mutex

## std::lock

## std::condition_variable

## std::future && std::promise

## async