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
thread是C++线程类，libcxx中[std::thread](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/thread#L216)的定义如下:
```cc
class _LIBCPP_TYPE_VIS thread {
    __libcpp_thread_t __t_;

    thread(const thread&);
    thread& operator=(const thread&);
public:
    template <class _Fp, class ..._Args,
              class = typename enable_if
              <
                   !is_same<typename __uncvref<_Fp>::type, thread>::value
              >::type
             >
    _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
    explicit thread(_Fp&& __f, _Args&&... __args);

    _LIBCPP_INLINE_VISIBILITY
    thread(thread&& __t) _NOEXCEPT : __t_(__t.__t_) {__t.__t_ = _LIBCPP_NULL_THREAD;}
    _LIBCPP_INLINE_VISIBILITY
    thread& operator=(thread&& __t) _NOEXCEPT;

    _LIBCPP_INLINE_VISIBILITY
    void swap(thread& __t) _NOEXCEPT {_VSTD::swap(__t_, __t.__t_);}

    _LIBCPP_INLINE_VISIBILITY
    bool joinable() const _NOEXCEPT {return !__libcpp_thread_isnull(&__t_);}
    void join();
    void detach();
    _LIBCPP_INLINE_VISIBILITY
    id get_id() const _NOEXCEPT {return __libcpp_thread_get_id(&__t_);}
    _LIBCPP_INLINE_VISIBILITY
    native_handle_type native_handle() _NOEXCEPT {return __t_;}

    static unsigned hardware_concurrency() _NOEXCEPT;
}
```
f表示线程的入口程序，args表示传给f的参数。当新的std::thread对象被构造，并且和入口程序关联后。新的线程就开始执行。
```cc
//线程内部的入口函数调用逻辑
std::invoke(decay_copy(std::forward<Function>(f)), decay_copy(std::forward<Args>(args))...);
```
注意:
- 线程只能移动，不能拷贝。
- 如果thread构造函数的第一个参数类型_Fp是thread类型，那么这里的模板实例化将被SFINAE掉。因为这里没有其他模板能匹配，所以就会编译报错找不到匹配的模板。这里是利用std::enable_if和std::is_same来进行SFINAE的，就是说这个模板构造函数的第一个参数类型不能是thread类型。
- 这里往线程入口函数__f传的参数都是按照移动或者拷贝来传的，因为参数传递的时候进行了decay_copy，退化了引用。所以如果需要往函数里面传引用，必须要程序员手动用std::ref或者std::cref来包裹参数传递给线程函数。(C++的设计者认为std::bind和std::thread默认应该采用拷贝，如果有使用需求，加上std::ref即可实现按引用传递)
- thread对象一定要join或者detach，不然会导致资源泄露(类似于new了一个对象就撒手不管了)。使用join或者detach之前用joinable来判断是否可以join。
- thread将忽略来自入口函数的任何返回值。如果有入口函数里面有异常抛出，则调用std::terminate。为将返回值或者异常传递给调用方线程，可使用std::promise或std::async。  
使用例子:
```cc
#include <iostream>
#include <thread>
using namespace std;
int main() {
    auto func = []() {
        for (int i = 0; i < 10; ++i) {
            cout << i << " ";
        }
        cout << endl;
    };
    std::thread t(func);
    if (t.joinable()) {
        t.detach();
    }
    auto func1 = [](int k) {
        for (int i = 0; i < k; ++i) {
            cout << i << " ";
        }
        cout << endl;
    };
    std::thread tt(func1, 20);
    cout << "当前线程ID " << tt.get_id() << endl;
    cout << "当前CPU硬件并发核心数 " << std::thread::hardware_concurrency() << endl;
    auto handle = tt.native_handle();// 类unix操作系统下handle 可用于 pthread 相关操作

    if (tt.joinable()) { // 检查线程可否被join
        tt.join();
    }
    return 0;
}
```


## std::mutex

## std::lock

## std::condition_variable

## std::future && std::promise

## async