## 原子操作
原子操作表示不会被线程调度(那么进程调度呢？)打断的操作，操作从开始到结束期间不会有其他的线程切换，把多个操作步骤看作一个不可分割的整体。
原子操作有两个属性
- 由于原子操作是不可分的，因此，来自不同线程的同意对象的第二个原子操作只能在第一个原子操作之前或之后获取该对象的状态。
- 基于其memory_order参数，原子操作可以针对同一个线程中其他原子操作的影响可见性建立排序要求。因此，它会抑制违反排序要求的编译器优化。  
使用原子操作可以不需要使用互斥量就能做好数据同步。
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

标准原子类型不能进行拷贝和赋值，它们没有拷贝构造函数和拷贝赋值运算符。标准原子类型还具有`is_lock_free()`成员函数(`std::atomic_flag`除外)，如果返回`true`表示这个原子类型内部是用原子指令来实现的原子操作，返回`false`表示原子内部是用的互斥量来模拟的原子操作，具体实现跟类型和编译的目标硬件有关。原子类型支持`load()`、`store()`、`exchange()`、`compare_exchange_weak()`、`compare_exchange_strong()`、`fetch_add()`、`fetch_sub()`、`fetch_and()`、`fetch_or()`、`fetch_xor()`操作
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
thread是C++线程类，类thread表示单个执行线程，线程允许多个函数同时执行。libcxx中[std::thread](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/thread#L216)的定义如下:
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
std::invoke(decay_copy(std::forward<_Fp>(__f)), decay_copy(std::forward<_Args>(__args))...);
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
    // 类unix操作系统下handle 可用于 pthread 相关操作,handle就是操作系统级别的线程句柄，类unix下就是线程描述符pthread_t(windows俺也不懂)
    auto handle = tt.native_handle();

    if (tt.joinable()) { // 检查线程可否被join
        tt.join();
    }
    return 0;
}
```

## std::this_thread
this_thread并不是一个类型，而是一个命名空间，包含一系列访问当前调用者线程的函数。
```cc
namespace this_thread
{

_LIBCPP_FUNC_VIS void sleep_for(const chrono::nanoseconds& __ns);

template <class _Rep, class _Period>
void
sleep_for(const chrono::duration<_Rep, _Period>& __d)
{
    using namespace chrono;
    if (__d > duration<_Rep, _Period>::zero())
    {
#if defined(_LIBCPP_COMPILER_GCC) && (__powerpc__ || __POWERPC__)
    //  GCC's long double const folding is incomplete for IBM128 long doubles.
        _LIBCPP_CONSTEXPR duration<long double> _Max = nanoseconds::max();
#else
        _LIBCPP_CONSTEXPR duration<long double> _Max = duration<long double>(ULLONG_MAX/1000000000ULL) ;
#endif
        nanoseconds __ns;
        if (__d < _Max)
        {
            __ns = duration_cast<nanoseconds>(__d);
            if (__ns < __d)
                ++__ns;
        }
        else
            __ns = nanoseconds::max();
        sleep_for(__ns);
    }
}

template <class _Clock, class _Duration>
void
sleep_until(const chrono::time_point<_Clock, _Duration>& __t)
{
    using namespace chrono;
    mutex __mut;
    condition_variable __cv;
    unique_lock<mutex> __lk(__mut);
    while (_Clock::now() < __t)
        __cv.wait_until(__lk, __t);
}

template <class _Duration>
inline _LIBCPP_INLINE_VISIBILITY
void
sleep_until(const chrono::time_point<chrono::steady_clock, _Duration>& __t)
{
    using namespace chrono;
    sleep_for(__t - steady_clock::now());
}

inline _LIBCPP_INLINE_VISIBILITY
void yield() _NOEXCEPT {__libcpp_thread_yield();}

inline _LIBCPP_INLINE_VISIBILITY
__thread_id
get_id() _NOEXCEPT
{
    return __libcpp_thread_get_current_id();
}

}  // this_thread
```
- get_id获取当前线程id。
- yield当前线程放弃CPU占有权，允许其他线程运行，该线程回到准备状态，重新分配资源。调用该方法后，可能执行其他线程，也可能还是执行该线程，这个完全依赖具体操作系统的线程调度机制的实现和当前的系统状态。libcxx实现下yield内部是shed_yield()，sched_yield()这个函数可以使用另一个级别等于或高于当前线程的线程先运行。如果没有符合条件的线程，那么这个函数将会立刻返回然后继续执行当前线程的程序。有策略的调用sched_yield()能在资源竞争情况很严重时，通过给其他的线程或进程运行机会的方式来提升程序的性能。
- sleep_for阻塞当前线程一段时间。
- sleep_until阻塞当前线程直到某个时间点。
## std::mutex相关
在多线程程序中经常会出现数据竞争问题，存在数据竞争的程序片段叫做临界区，此时可以用一种叫做互斥量的东西来对资源进行互斥上锁，互斥量同时只允许一个线程持有，持有互斥量的线程就能访问存在数据竞争的临界区。
### std::mutex
最基本的互斥量就是std::mutex，独占的互斥量，不能递归使用，不带超时功能。(一般不直接使用std::mutex)
```cc
class _LIBCPP_TYPE_VIS _LIBCPP_THREAD_SAFETY_ANNOTATION(capability("mutex")) mutex
{
    __libcpp_mutex_t __m_ = _LIBCPP_MUTEX_INITIALIZER;

public:
    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR mutex() = default;

    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

#if defined(_LIBCPP_HAS_TRIVIAL_MUTEX_DESTRUCTION)
    ~mutex() = default;
#else
    ~mutex() _NOEXCEPT;
#endif

    void lock() _LIBCPP_THREAD_SAFETY_ANNOTATION(acquire_capability());
    bool try_lock() _NOEXCEPT _LIBCPP_THREAD_SAFETY_ANNOTATION(try_acquire_capability(true));
    void unlock() _NOEXCEPT _LIBCPP_THREAD_SAFETY_ANNOTATION(release_capability());

    typedef __libcpp_mutex_t* native_handle_type;
    _LIBCPP_INLINE_VISIBILITY native_handle_type native_handle() {return &__m_;}
};
```
注意:
- std::mutex不可复制不可移动。
- 调用方线程从成功调用lock或者try_lock开始就持有互斥量，直到调用unlock才释放互斥量。
- 调用lock的时候，如果这个mutex已经被其他线程持有，那么lock就会阻塞在这里，直到持有mutex的线程调用unlock。如果通过用try_lock来尝试持有一个已经被其他线程持有的锁，那么try_lock不会阻塞，而是会返回false。  

使用例子:
```cc
#include <iostream>
#include <mutex>
#include <thread>
using namespace std;
std::mutex mutex_;
int main() {
    auto func1 = [](int k) {
        mutex_.lock();
        for (int i = 0; i < k; ++i) {
            cout << i << " ";
        }
        cout << endl;
        mutex_.unlock();
    };
    std::thread threads[5];
    for (int i = 0; i < 5; ++i) {
        threads[i] = std::thread(func1, 200);
    }
    for (auto& th : threads) {
        th.join();
    }
    return 0;
}
```
### std::recursive_mutex
现在有这么一种情况:  
线程1，临界区A，临界区B和互斥量m，其中临界区A和临界区B都需要持有互斥量m(也就是进入临界区之前要执行加锁操作)才能访问。如果线程1访问临界区A，但是临界区A里面的代码对临界区B里面的代码进行了调用。那么就会出现在线程1想进入B的时候没法对m进行加锁操作，就会导致死锁(lock和try_lock都会)，因为线程1没有释放m，但是线程1又想对m再次加锁。这种情况下，普通的互斥量std::mutex已经不能满足使用了，可以使用std::recursive_mutex来让同一个线程多次上锁。std::recursive_mutex如下:
```cc
class _LIBCPP_TYPE_VIS recursive_mutex
{
    __libcpp_recursive_mutex_t __m_;

public:
     recursive_mutex();
     ~recursive_mutex();

private:
    recursive_mutex(const recursive_mutex&); // = delete;
    recursive_mutex& operator=(const recursive_mutex&); // = delete;

public:
    void lock();
    bool try_lock() _NOEXCEPT;
    void unlock()  _NOEXCEPT;

    typedef __libcpp_recursive_mutex_t* native_handle_type;

    _LIBCPP_INLINE_VISIBILITY
    native_handle_type native_handle() {return &__m_;}
};
```
通过阅读libcxx源代码可以查看到std::mutex和std::recursive_mutex内部系统级别的互斥量描述符都是pthread_mutex_t，而且进行加锁解锁操作都是通过系统级别的pthread_mutex_lock/pthread_mutex_trylock和pthread_mutex_unlock，那么是哪里不一样导致的std::mutex只能上锁一次而std::recursive_mutex却能锁上加锁？其实就是在他们内部的__m_初始化方式不一样，std::mutex是直接通过PTHREAD_MUTEX_INITIALIZER来初始化__m_的，而std::recursive_mutex则是通过一系列操作来为__m_设置额外PTHREAD_MUTEX_RECURSIVE属性。所以std::recursive_mutex具备了锁上加锁的能力(可重入性/递归锁)。
```cc
int __libcpp_recursive_mutex_init(__libcpp_recursive_mutex_t *__m)
{
    pthread_mutexattr_t attr;
    int __ec = pthread_mutexattr_init(&attr);
    if (__ec)
        return __ec;
    //这里为recursive_mutex赋予了递归加锁的能力
    __ec = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (__ec) {
        pthread_mutexattr_destroy(&attr);
        return __ec;
    }
    __ec = pthread_mutex_init(__m, &attr);
    if (__ec) {
        pthread_mutexattr_destroy(&attr);
        return __ec;
    }
    __ec = pthread_mutexattr_destroy(&attr);
    if (__ec) {
        pthread_mutex_destroy(__m);
        return __ec;
    }
    return 0;
}
```
使用std::recursive_mutex的例子:
```cc
#include <iostream>
#include <thread>
#include <mutex>
 
class X {
    std::recursive_mutex m;
    std::string shared;
  public:
    void fun1() {
      std::lock_guard<std::recursive_mutex> lk(m);
      shared = "fun1";
      std::cout << "in fun1, shared variable is now " << shared << '\n';
    }
    void fun2() {
      std::lock_guard<std::recursive_mutex> lk(m);
      shared = "fun2";
      std::cout << "in fun2, shared variable is now " << shared << '\n';
      fun1(); // recursive lock becomes useful here
      std::cout << "back in fun2, shared variable is " << shared << '\n';
    };
};
 
int main() 
{
    X x;
    std::thread t1(&X::fun1, &x);
    std::thread t2(&X::fun2, &x);
    t1.join();
    t2.join();
}
```
需要注意的一点是对std::recursive_mutex加锁多少次就需要对它进行相应次数的解锁，不然这个线程依旧持有该互斥量。
### std::timed_mutex
std::timed_mutex在std::mutex的基本互斥和同步基础上支持超时机制，std::timed_mutex类型新增try_lock_for和try_lock_until成员函数，可以在一段时间内尝试获取量或者在指定时间点之前获取互斥量上锁。
```cc
class _LIBCPP_TYPE_VIS timed_mutex
{
    mutex              __m_;
    condition_variable __cv_;
    bool               __locked_;
public:
     timed_mutex();
     ~timed_mutex();

private:
    timed_mutex(const timed_mutex&); // = delete;
    timed_mutex& operator=(const timed_mutex&); // = delete;

public:
    void lock();
    bool try_lock() _NOEXCEPT;
    template <class _Rep, class _Period>
        _LIBCPP_INLINE_VISIBILITY
        bool try_lock_for(const chrono::duration<_Rep, _Period>& __d)
            {return try_lock_until(chrono::steady_clock::now() + __d);}
    template <class _Clock, class _Duration>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __t);
    void unlock() _NOEXCEPT;
};
template <class _Clock, class _Duration>
bool timed_mutex::try_lock_until(const chrono::time_point<_Clock, _Duration>& __t)
{
    using namespace chrono;
    unique_lock<mutex> __lk(__m_);
    bool no_timeout = _Clock::now() < __t;
    while (no_timeout && __locked_)
        no_timeout = __cv_.wait_until(__lk, __t) == cv_status::no_timeout;
    if (!__locked_)
    {
        __locked_ = true;
        return true;
    }
    return false;
}

```
上面代码可以看到超时机制是利用的条件变量的wait_until来对互斥锁进行等待。调用try_lock_for或者try_lock_until的线程都可以在等待时间内对互斥量进行等待，如果在时间内有其他线程释放了该互斥量，那么该线程就会获得对互斥量的锁。如果等待时间结束，那么就会返回false，不再去尝试对互斥量上锁，程序会继续往下执行。  
使用例子:
```cc
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>

std::timed_mutex mtx;

void fireworks() {
    // waiting to get a lock: each thread prints "-" every 200ms:
    while (!mtx.try_lock_for(std::chrono::milliseconds(200))) {
        std::cout << "-";
    }
    // got a lock! - wait for 1s, then this thread prints "*"
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "*\n";
    mtx.unlock();
}

int main ()
{
    std::thread threads[10];
    // spawn 10 threads:
    for (int i=0; i<10; ++i)
        threads[i] = std::thread(fireworks);

    for (auto& th : threads) th.join();

    return 0;
}
```
### std::recursive_timed_mutex
std::recursive_timed_mutex在std::recursive_mutex的基本互斥和同步基础上支持超时机制，std::timed_mutex类型新增try_lock_for和try_lock_until成员函数，可以在一段时间内尝试获取量或者在指定时间点之前获取互斥量上锁。
```cc
class _LIBCPP_TYPE_VIS recursive_timed_mutex
{
    mutex              __m_;
    condition_variable __cv_;
    size_t             __count_;
    __thread_id        __id_;
public:
    recursive_timed_mutex();
    ~recursive_timed_mutex();

private:
    recursive_timed_mutex(const recursive_timed_mutex&); // = delete;
    recursive_timed_mutex& operator=(const recursive_timed_mutex&); // = delete;

public:
    void lock();
    bool try_lock() _NOEXCEPT;
    template <class _Rep, class _Period>
        _LIBCPP_INLINE_VISIBILITY
        bool try_lock_for(const chrono::duration<_Rep, _Period>& __d)
            {return try_lock_until(chrono::steady_clock::now() + __d);}
    template <class _Clock, class _Duration>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __t);
    void unlock() _NOEXCEPT;
};
template <class _Clock, class _Duration>
bool
recursive_timed_mutex::try_lock_until(const chrono::time_point<_Clock, _Duration>& __t)
{
    using namespace chrono;
    __thread_id __id = this_thread::get_id();
    unique_lock<mutex> lk(__m_);
    if (__id == __id_)
    {
        if (__count_ == numeric_limits<size_t>::max())
            return false;
        ++__count_;
        return true;
    }
    bool no_timeout = _Clock::now() < __t;
    while (no_timeout && __count_ != 0)
        no_timeout = __cv_.wait_until(lk, __t) == cv_status::no_timeout;
    if (__count_ == 0)
    {
        __count_ = 1;
        __id_ = __id;
        return true;
    }
    return false;
}
```
std::recursive_timed_mutex和std::recursive_mutex一样具有可重入性，并且和std::timed_mutex一样具有超时性。
### std::shared_mutex (C++17)

### std::shared_timed_mutex (C++17)
## std::lock
假设有两个互斥量m1,m2，一个线程先锁住m1再锁住m2，另一个线程先锁住m2再锁住m1,就有可能出现死锁。std::lock内部使用了死锁避免的算法，可以有效避免死锁，一次锁住多个互斥量或者可锁定对象(必须满足有try_lock函数):
```cc
//对mutex进行上锁
std::lock(m1, m2);

//对unique_lock进行上锁
std::unique_lock lock1(m1, std::defer_lock);
std::unique_lock lock2(m2, std::defer_lock);
std::lock(lock1, lock2);
```

[std::lock实现](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/mutex#L446)

这里介绍两种RAII方式的锁封装，防止线程由于编码失误忘记手动释放锁导致一直持有锁。
### std::lock_guard
lock_guard就是采用RAII在构造的时候上锁，析构的时候释放锁
```cc
template <class _Mutex>
class _LIBCPP_TEMPLATE_VIS _LIBCPP_THREAD_SAFETY_ANNOTATION(scoped_lockable)
lock_guard
{
public:
    typedef _Mutex mutex_type;

private:
    mutex_type& __m_;
public:

    _LIBCPP_NODISCARD_EXT _LIBCPP_INLINE_VISIBILITY
    //上锁
    explicit lock_guard(mutex_type& __m) _LIBCPP_THREAD_SAFETY_ANNOTATION(acquire_capability(__m))
        : __m_(__m) {__m_.lock();}

    _LIBCPP_NODISCARD_EXT _LIBCPP_INLINE_VISIBILITY
    lock_guard(mutex_type& __m, adopt_lock_t) _LIBCPP_THREAD_SAFETY_ANNOTATION(requires_capability(__m))
        : __m_(__m) {}
    _LIBCPP_INLINE_VISIBILITY
    //释放锁
    ~lock_guard() _LIBCPP_THREAD_SAFETY_ANNOTATION(release_capability()) {__m_.unlock();}

private:
    lock_guard(lock_guard const&) _LIBCPP_EQUAL_DELETE;
    lock_guard& operator=(lock_guard const&) _LIBCPP_EQUAL_DELETE;
};

```
注意:  
lock_guard不可复制  
使用例子:  
```cc
#include <thread>
#include <mutex>
#include <iostream>
 
int g_i = 0;
std::mutex g_i_mutex;  // 保护 g_i
 
void safe_increment()
{
    std::lock_guard<std::mutex> lock(g_i_mutex);
    ++g_i;
 
    std::cout << std::this_thread::get_id() << ": " << g_i << '\n';
 
    // g_i_mutex 在锁离开作用域时自动释放
}
 
int main()
{
    std::cout << "main: " << g_i << '\n';
 
    std::thread t1(safe_increment);
    std::thread t2(safe_increment);
 
    t1.join();
    t2.join();
 
    std::cout << "main: " << g_i << '\n';
}
```
## std::condition_variable

## std::future && std::promise

## async

## 自旋锁