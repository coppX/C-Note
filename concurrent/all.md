# thread相关
### std::thread
thread是C++线程类，类thread表示单个执行线程，线程允许多个函数同时执行。libcxx中[std::thread](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/thread#L216)的定义如下:
```cpp
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
```cpp
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
```cpp
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
### std::this_thread
this_thread并不是一个类型，而是一个命名空间，包含一系列访问当前调用者线程的函数。
```cpp
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
# mutex相关
在多线程程序中经常会出现数据竞争问题，存在数据竞争的程序片段叫做临界区，此时可以用一种叫做互斥量的东西来对资源进行互斥上锁，互斥量同时只允许一个线程持有，持有互斥量的线程就能访问存在数据竞争的临界区。
### std::mutex
最基本的互斥量就是std::mutex，独占的互斥量，不能递归使用，不带超时功能。(一般不直接使用std::mutex)
```cpp
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
这里的lock定义后面，有个acquire_capability，包括下面的try_acquire_capability和release_capability以及类名定义处的capability("mutex")，这是clang编译器的特性，是clang的线程安全静态分析选项结合使用的代码注释, [详情参考](https://clang.llvm.org/docs/ThreadSafetyAnalysis.html)
使用例子:
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
#if _LIBCPP_STD_VER > 14
class _LIBCPP_TYPE_VIS _LIBCPP_AVAILABILITY_SHARED_MUTEX shared_mutex
{
    __shared_mutex_base __base;
public:
    _LIBCPP_INLINE_VISIBILITY shared_mutex() : __base() {}
    _LIBCPP_INLINE_VISIBILITY ~shared_mutex() = default;

    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

    // Exclusive ownership
    _LIBCPP_INLINE_VISIBILITY void lock()     { return __base.lock(); }
    _LIBCPP_INLINE_VISIBILITY bool try_lock() { return __base.try_lock(); }
    _LIBCPP_INLINE_VISIBILITY void unlock()   { return __base.unlock(); }

    // Shared ownership
    _LIBCPP_INLINE_VISIBILITY void lock_shared()     { return __base.lock_shared(); }
    _LIBCPP_INLINE_VISIBILITY bool try_lock_shared() { return __base.try_lock_shared(); }
    _LIBCPP_INLINE_VISIBILITY void unlock_shared()   { return __base.unlock_shared(); }

//     typedef __shared_mutex_base::native_handle_type native_handle_type;
//     _LIBCPP_INLINE_VISIBILITY native_handle_type native_handle() { return __base::unlock_shared(); }
};
#endif
```
shared_mutex就是我们常说的读写锁，支持独占性加锁和共享性加锁。当我们读临界区的时候，就需要用到shared_mutex的读，使用lock_shared,try_lock_shared和unlock_shared，可以对锁进行共享式上锁解锁，这个时候就可以有多个线程同时访问临界区。当我们对临界区进行写的时候，使用lock, try_lock，unlock对临界区独占加锁解锁，使得只有一个线程独占这个临界区。他内部的实现是封装在一个__shared_mutex_base类型的成员中，其实现如下:
```cpp
struct _LIBCPP_TYPE_VIS _LIBCPP_AVAILABILITY_SHARED_MUTEX _LIBCPP_THREAD_SAFETY_ANNOTATION(capability("shared_mutex"))
__shared_mutex_base
{
    mutex               __mut_;
    condition_variable  __gate1_;
    condition_variable  __gate2_;
    unsigned            __state_;

    static const unsigned __write_entered_ = 1U << (sizeof(unsigned)*__CHAR_BIT__ - 1); // 1000 0000 0000 0000 0000 0000 0000 0000
    static const unsigned __n_readers_ = ~__write_entered_;                             // 0111 1111 1111 1111 1111 1111 1111 1111

    __shared_mutex_base();
    _LIBCPP_INLINE_VISIBILITY ~__shared_mutex_base() = default;

    __shared_mutex_base(const __shared_mutex_base&) = delete;
    __shared_mutex_base& operator=(const __shared_mutex_base&) = delete;

    // Exclusive ownership
    void lock() _LIBCPP_THREAD_SAFETY_ANNOTATION(acquire_capability()); // blocking
    bool try_lock() _LIBCPP_THREAD_SAFETY_ANNOTATION(try_acquire_capability(true));
    void unlock() _LIBCPP_THREAD_SAFETY_ANNOTATION(release_capability());

    // Shared ownership
    void lock_shared() _LIBCPP_THREAD_SAFETY_ANNOTATION(acquire_shared_capability()); // blocking
    bool try_lock_shared() _LIBCPP_THREAD_SAFETY_ANNOTATION(try_acquire_shared_capability(true));
    void unlock_shared() _LIBCPP_THREAD_SAFETY_ANNOTATION(release_shared_capability());

//     typedef implementation-defined native_handle_type; // See 30.2.3
//     native_handle_type native_handle(); // See 30.2.3
};
```
分析一下读写锁的实现，首先看看独占的接口
```cpp
void
__shared_mutex_base::lock()
{
    unique_lock<mutex> lk(__mut_);
    while (__state_ & __write_entered_)
        __gate1_.wait(lk);
    __state_ |= __write_entered_;
    while (__state_ & __n_readers_)
        __gate2_.wait(lk);
}

bool
__shared_mutex_base::try_lock()
{
    unique_lock<mutex> lk(__mut_);
    if (__state_ == 0)
    {
        __state_ = __write_entered_;
        return true;
    }
    return false;
}

void
__shared_mutex_base::unlock()
{
    lock_guard<mutex> _(__mut_);
    __state_ = 0;
    __gate1_.notify_all();
}
```
第一次调用lock，最开始__state_为0，__write_entered_为0x80000000，不会进入第一个while， __state_变成0x80000000，而__n_readers_为0x7fffffff，同样不会进入第二个循环, 第一次lock是不会进行wait操作的, 条件变量__gate1_和__gate2_都没有被启用。如果这个时候没有调用unlock，__state_就不会被重置，继续调用lock，那么就会进入第一个while，表示已经进入了写锁的状态，__gate1.wait就会阻塞在这里等待，除非调用一次unlock来对__gate1_调用notify_all进行唤醒，也就是释放掉写锁才能重新再次加锁。这里如果能进入第二个，表示shared_mutex已经是处于读锁的状态了，就应该把这里的读锁__gate2给阻塞起来，除非在unlock_shared里面把所有的读锁都释放掉，才能唤醒__gate2_。在加上了写锁(__state_ != 0)的情况下，__gate1和__gate2都会阻塞在这里。 
try_lock就是如果__state == 0的情况下就表示没有加锁，可以进行上锁操作，参考上面描述的第一次lock，__state_为0，否则就加锁失败。  
unlock重置掉__state_，并且唤醒阻塞的__gate1_。  
__gate1_表示是的独占状态下对加锁的阻塞和记录读锁数量满了的情况下的阻塞，__gate2_表示的是共享状态下对加锁的阻塞，__state_用来记录是否处于写锁，和有多少读锁。   
下面看看读写锁里面的共享加锁操作:
```cpp
void
__shared_mutex_base::lock_shared()
{
    unique_lock<mutex> lk(__mut_);
    while ((__state_ & __write_entered_) || (__state_ & __n_readers_) == __n_readers_)
        __gate1_.wait(lk);
    unsigned num_readers = (__state_ & __n_readers_) + 1;
    __state_ &= ~__n_readers_;
    __state_ |= num_readers;
}

bool
__shared_mutex_base::try_lock_shared()
{
    unique_lock<mutex> lk(__mut_);
    unsigned num_readers = __state_ & __n_readers_;
    if (!(__state_ & __write_entered_) && num_readers != __n_readers_)
    {
        ++num_readers;
        __state_ &= ~__n_readers_;
        __state_ |= num_readers;
        return true;
    }
    return false;
}

void
__shared_mutex_base::unlock_shared()
{
    lock_guard<mutex> _(__mut_);
    unsigned num_readers = (__state_ & __n_readers_) - 1;
    __state_ &= ~__n_readers_;
    __state_ |= num_readers;
    if (__state_ & __write_entered_)
    {
        if (num_readers == 0)
            __gate2_.notify_one();
    }
    else
    {
        if (num_readers == __n_readers_ - 1)
            __gate1_.notify_one();
    }
}
```
首先看lock_shared，while的判断条件中如果__state & __write_entered_ != 0就表示已经加上了写锁，需要阻塞在__gate1._上，或者(__state_ & __n_readers_) == __n_readers_的情况下也会阻塞在__gate1_上(读的数量已经达到最大了，不能再加了，记录不下了)。等shared_mutex调用unlock释放写锁来唤醒__gate1_或者调用unlock_shared来唤醒__gate1_。其他情况，只需要增加reader的数量就ok，并记录读的数量到__state_中。  
try_lock_shared类似于lock_shared的判断条件，判断一下是否处于非独占状态，并且读锁的数量没有记录满的情况下，就可以对其进行try_lock_shared，否则就不行。  
unlock_shared首先会减少记录的读的数量，进入__state_ & __write_entered_判断后，表示目前处于写锁的状态，那么这个时候调用unlock_shared可以对lock里面阻塞的__gate2_进行唤醒。否则，如果num_readers == __n_readers_ - 1就表示释放掉一个读锁后，可以记录新的读锁，就可以唤醒lock_shared里面阻塞的_gate1_。类似于消费者消费掉了一个数量后，我们唤醒生产者继续生产。

### std::shared_timed_mutex (C++17)
shared_timed_mutex就是shared_mutex和timed_mutex的结合体，同时具备读写锁和支持超时机制。
```cpp
class _LIBCPP_TYPE_VIS _LIBCPP_AVAILABILITY_SHARED_MUTEX shared_timed_mutex
{
    __shared_mutex_base __base;
public:
    shared_timed_mutex();
    _LIBCPP_INLINE_VISIBILITY ~shared_timed_mutex() = default;

    shared_timed_mutex(const shared_timed_mutex&) = delete;
    shared_timed_mutex& operator=(const shared_timed_mutex&) = delete;

    // Exclusive ownership
    void lock();
    bool try_lock();
    template <class _Rep, class _Period>
        _LIBCPP_INLINE_VISIBILITY
        bool
        try_lock_for(const chrono::duration<_Rep, _Period>& __rel_time)
        {
            return try_lock_until(chrono::steady_clock::now() + __rel_time);
        }
    template <class _Clock, class _Duration>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        bool
        try_lock_until(const chrono::time_point<_Clock, _Duration>& __abs_time);
    void unlock();

    // Shared ownership
    void lock_shared();
    bool try_lock_shared();
    template <class _Rep, class _Period>
        _LIBCPP_INLINE_VISIBILITY
        bool
        try_lock_shared_for(const chrono::duration<_Rep, _Period>& __rel_time)
        {
            return try_lock_shared_until(chrono::steady_clock::now() + __rel_time);
        }
    template <class _Clock, class _Duration>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        bool
        try_lock_shared_until(const chrono::time_point<_Clock, _Duration>& __abs_time);
    void unlock_shared();
};

shared_timed_mutex::shared_timed_mutex() : __base() {}
void shared_timed_mutex::lock()     { return __base.lock(); }
bool shared_timed_mutex::try_lock() { return __base.try_lock(); }
void shared_timed_mutex::unlock()   { return __base.unlock(); }
void shared_timed_mutex::lock_shared() { return __base.lock_shared(); }
bool shared_timed_mutex::try_lock_shared() { return __base.try_lock_shared(); }
void shared_timed_mutex::unlock_shared() { return __base.unlock_shared(); }

```
支持的接口包括，独占型：lock, try_lock, try_lock_for, try_lock_until。共享型: lock_shared, try_lock_shared, try_lock_shared_for, try_lock_shared_until。  
其中，简单的加锁解锁操作都是利用的base来实现的，包括lock, try_lock, unlock, lock_shared, try_lock_shared, unlock_shared，和上面的shared_mutex类似的实现方式。  
而其中带有时间的接口，try_lock_for和try_lock_shared_for是依赖于另外一个until接口，try_lock_for是依赖try_lock_until来实现的，try_lock_shared_for是依赖的try_lock_shared_until来实现的。其实现如下:
```cpp

template <class _Clock, class _Duration>
bool
shared_timed_mutex::try_lock_until(
                        const chrono::time_point<_Clock, _Duration>& __abs_time)
{
    unique_lock<mutex> __lk(__base.__mut_);
    if (__base.__state_ & __base.__write_entered_)
    {
        while (true)
        {
            cv_status __status = __base.__gate1_.wait_until(__lk, __abs_time);
            if ((__base.__state_ & __base.__write_entered_) == 0)
                break;
            if (__status == cv_status::timeout)
                return false;
        }
    }
    __base.__state_ |= __base.__write_entered_;
    if (__base.__state_ & __base.__n_readers_)
    {
        while (true)
        {
            cv_status __status = __base.__gate2_.wait_until(__lk, __abs_time);
            if ((__base.__state_ & __base.__n_readers_) == 0)
                break;
            if (__status == cv_status::timeout)
            {
                __base.__state_ &= ~__base.__write_entered_;
                __base.__gate1_.notify_all();
                return false;
            }
        }
    }
    return true;
}
```
首先来看上面这个if，如果之前没有使用过lock操作，那么第一次进入这个函数，if判断__base.__state_ & __base.__write_entered_是进不去的，__state为0。然后下一句就会把__write_entered_设置给__state_，也不会进入第二个if判断，立马返回true, 独占加锁成功。我们第二次再来调用这个try_lock_until(也就是在独占加锁未释放的情况下)，那么第一个if是能进入的，我们就在while里面一直对__gate1_等待，如果某一次循环后(__base.__state_ & __base.__write_entered_) == 0条件满足，就表示在其他地方重置了__state_，也就是调用了unlock操作，这里就能加锁成功，break，跳出while死循环，返回true。如果等待的结果是timeout，就表示已经等待超时了，我们不需要再等待了，直接返回false即可。  
第二个if表示我们在有读锁的情况下，进行独占加锁的操作，那么我们就会对__gate2_进行阻塞等待，如果某次__gate2_醒来后发现，(__base.__state_ & __base.__n_readers_) == 0成立，也就是说没有读锁了，只有写锁，那么我们就不需要对__gate2_进行等待操作了，直接break, 返回true，独占加锁成功， 已经没有共享加锁了。如果等待的结果是timeout，那么已经等待超时了，我们取消掉独占加锁(__base.__state_ &= ~__base.__write_entered_)，并且尝试唤醒__gate1_，返回false，加锁失败。
```cpp
template <class _Clock, class _Duration>
bool
shared_timed_mutex::try_lock_shared_until(
                        const chrono::time_point<_Clock, _Duration>& __abs_time)
{
    unique_lock<mutex> __lk(__base.__mut_);
    if ((__base.__state_ & __base.__write_entered_) || (__base.__state_ & __base.__n_readers_) == __base.__n_readers_)
    {
        while (true)
        {
            cv_status status = __base.__gate1_.wait_until(__lk, __abs_time);
            if ((__base.__state_ & __base.__write_entered_) == 0 &&
                                       (__base.__state_ & __base.__n_readers_) < __base.__n_readers_)
                break;
            if (status == cv_status::timeout)
                return false;
        }
    }
    unsigned __num_readers = (__base.__state_ & __base.__n_readers_) + 1;
    __base.__state_ &= ~__base.__n_readers_;
    __base.__state_ |= __num_readers;
    return true;
}
```
这里的if有两个判断条件，类似于try_lock_shared, 第一个表示处于独占加锁状态，第二个表示读锁的记录已满的状态，这两种状态都需要对__gate1_进行阻塞等待，如果__gate1_醒来后发现已经没有写锁了，并且还能记录得下新的读锁，那么就break跳出循环，更新记录写锁数量，然后返回true。如果等待的结果是超时，那么也别等了，已经加锁失败了，返回false。
# lock相关
### std::lock
假设有两个互斥量m1,m2，一个线程先锁住m1再锁住m2，另一个线程先锁住m2再锁住m1,就有可能出现死锁。std::lock内部使用了死锁避免的算法，可以有效避免死锁，一次锁住多个互斥量或者可锁定对象(必须满足有try_lock函数):
```cpp
template <class _L0, class _L1, class _L2, class ..._L3>
void lock(_L0& __l0, _L1& __l1, _L2& __l2, _L3& ...__l3)
```
```cpp
//对mutex进行上锁
std::lock(m1, m2);

//对unique_lock进行上锁
std::unique_lock lock1(m1, std::defer_lock);
std::unique_lock lock2(m2, std::defer_lock);
std::lock(lock1, lock2);
```
如果std::lock抛出异常，则会对之前已上锁的对象调用unlock解锁。  

[std::lock实现](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/mutex#L446)

### std::try_lock
std::try_lock[实现代码](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/mutex#L351)
```cpp
template <class _L0, class _L1>
int try_lock(_L0& __l0, _L1& __l1)
```
尝试给每个可锁定对象加锁，每个可加锁对象调用try_lock，如果try_lock失败，则不再进一步调用try_lock，并对已锁定的可锁定对象调用unlock，返回失败对象的下标(参数列表里的第几个)，如果try_lock过程抛出异常，在重新抛出异常之前对所有已锁定的对象调用unlock。如果所有参数加锁成功，返回-1。


std::lock和std::try_lock的区别是std::lock是阻塞式的std::try_lock是非阻塞式的。  


这里介绍几种RAII方式的锁封装，防止线程由于编码失误忘记手动释放锁导致一直持有锁。


### std::lock_guard
lock_guard就是采用RAII在构造的时候上锁，析构的时候释放锁
```cpp
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
```cpp
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
### std::scoped_lock(C++17)
这是利用的RAII机制在构造的时候对mutex上锁，析构的时候释放锁，是lock_guard的一种扩展形式，毕竟lock_guard只能对一个mutex上锁
```cpp
template <class ..._MArgs>
class _LIBCPP_TEMPLATE_VIS scoped_lock
{
    static_assert(sizeof...(_MArgs) > 1, "At least 2 lock types required");
    typedef tuple<_MArgs&...> _MutexTuple;

public:
    _LIBCPP_INLINE_VISIBILITY
    explicit scoped_lock(_MArgs&... __margs)
      : __t_(__margs...)
    {
        _VSTD::lock(__margs...);
    }

    _LIBCPP_INLINE_VISIBILITY
    scoped_lock(adopt_lock_t, _MArgs&... __margs)
        : __t_(__margs...)
    {
    }

    _LIBCPP_INLINE_VISIBILITY
    ~scoped_lock() {
        typedef typename __make_tuple_indices<sizeof...(_MArgs)>::type _Indices;
        __unlock_unpack(_Indices{}, __t_);
    }

    scoped_lock(scoped_lock const&) = delete;
    scoped_lock& operator=(scoped_lock const&) = delete;

private:
    template <size_t ..._Indx>
    _LIBCPP_INLINE_VISIBILITY
    static void __unlock_unpack(__tuple_indices<_Indx...>, _MutexTuple& __mt) {
        _VSTD::__unlock(_VSTD::get<_Indx>(__mt)...);
    }

    _MutexTuple __t_;
};
```
这里是选取的一个通用形式，支持变参模板，他还有一些特化的形式(template<> class scoped_lock<>, template<typename Mutex> class scoped_lock<Mutex>)。
### std::unique_lock
相比于上面的std::lock_guard,std::unique_lock更加的灵活。允许延迟锁定、锁定的有时限尝试、递归锁定、所有权转移和条件变量一同使用。
```cpp
template <class _Mutex>
class _LIBCPP_TEMPLATE_VIS unique_lock
{
public:
    typedef _Mutex mutex_type;

private:
    //互斥量
    mutex_type* __m_;
    //互斥量是否锁定
    bool __owns_;

public:
    _LIBCPP_INLINE_VISIBILITY
    unique_lock() _NOEXCEPT : __m_(nullptr), __owns_(false) {}
    _LIBCPP_INLINE_VISIBILITY
    explicit unique_lock(mutex_type& __m)
        : __m_(_VSTD::addressof(__m)), __owns_(true) {__m_->lock();}
    _LIBCPP_INLINE_VISIBILITY
    //如果第二个参数是defer_lock_t类型，表示互斥量处于解锁状态，则假定std::unique_lock实例构造的时候没有拥有该互斥量, __owns = false
    unique_lock(mutex_type& __m, defer_lock_t) _NOEXCEPT
        : __m_(_VSTD::addressof(__m)), __owns_(false) {}
    _LIBCPP_INLINE_VISIBILITY
    //如果第二个参数是try_to_lock_t类型，则假定构造时尝试用try_lock来锁定互斥量， __owns = __m.try_lock()
    unique_lock(mutex_type& __m, try_to_lock_t)
        : __m_(_VSTD::addressof(__m)), __owns_(__m.try_lock()) {}
    _LIBCPP_INLINE_VISIBILITY
    //如果第二个参数是adopt_lock_t类型，则假设当前unique_lock对象构造时已经锁定该互斥量，__owns = true
    unique_lock(mutex_type& __m, adopt_lock_t)
        : __m_(_VSTD::addressof(__m)), __owns_(true) {}
    template <class _Clock, class _Duration>
    _LIBCPP_INLINE_VISIBILITY
    //如果第二个参数是const chrono::time_point<_Clock, _Duration>类型，则假定构造时尝试用try_lock_until来锁定互斥量，__owns = __m.try_lock_until(__t)
        unique_lock(mutex_type& __m, const chrono::time_point<_Clock, _Duration>& __t)
            : __m_(_VSTD::addressof(__m)), __owns_(__m.try_lock_until(__t)) {}
    template <class _Rep, class _Period>
    _LIBCPP_INLINE_VISIBILITY
    //如果第二个参数是const chrono::duration<_Rep, _Period>类型，则假定构造时尝试用try_lock_for来锁定互斥量， __owns = __m.try_lock_for(__d)
        unique_lock(mutex_type& __m, const chrono::duration<_Rep, _Period>& __d)
            : __m_(_VSTD::addressof(__m)), __owns_(__m.try_lock_for(__d)) {}
    _LIBCPP_INLINE_VISIBILITY
    ~unique_lock()
    {   
        //如果互斥量处于锁定状态，那么在析构的时候才会去unlock
        if (__owns_)
            __m_->unlock();
    }

private:
    unique_lock(unique_lock const&); // = delete;
    unique_lock& operator=(unique_lock const&); // = delete;

public:
#ifndef _LIBCPP_CXX03_LANG
    _LIBCPP_INLINE_VISIBILITY
    //移动构造函数
    unique_lock(unique_lock&& __u) _NOEXCEPT
        : __m_(__u.__m_), __owns_(__u.__owns_)
        {__u.__m_ = nullptr; __u.__owns_ = false;}
    _LIBCPP_INLINE_VISIBILITY
    //移动赋值运算符
    unique_lock& operator=(unique_lock&& __u) _NOEXCEPT
        {
            //销毁之前先释放锁
            if (__owns_)
                __m_->unlock();
            __m_ = __u.__m_;
            __owns_ = __u.__owns_;
            __u.__m_ = nullptr;
            __u.__owns_ = false;
            return *this;
        }

#endif  // _LIBCPP_CXX03_LANG

    void lock();
    bool try_lock();

    template <class _Rep, class _Period>
        bool try_lock_for(const chrono::duration<_Rep, _Period>& __d);
    template <class _Clock, class _Duration>
        bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __t);

    void unlock();

    _LIBCPP_INLINE_VISIBILITY
    void swap(unique_lock& __u) _NOEXCEPT
    {
        _VSTD::swap(__m_, __u.__m_);
        _VSTD::swap(__owns_, __u.__owns_);
    }
    _LIBCPP_INLINE_VISIBILITY
    mutex_type* release() _NOEXCEPT
    {
        mutex_type* __m = __m_;
        __m_ = nullptr;
        __owns_ = false;
        return __m;
    }

    _LIBCPP_INLINE_VISIBILITY
    bool owns_lock() const _NOEXCEPT {return __owns_;}
    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_EXPLICIT
        operator bool () const _NOEXCEPT {return __owns_;}
    _LIBCPP_INLINE_VISIBILITY
    mutex_type* mutex() const _NOEXCEPT {return __m_;}
};
```

从上面源代码可以看出来std::unique确实是适用所有类型的互斥量，从成员函数看出来可以支持的互斥量类型:  
- lock/try_lock/unlock:所有的互斥量(可锁定)
- try_lock_for/try_lock_until:支持std::timed_mutex、std::recursive_timed_mutex、std::shared_timed_mutex(可定时锁定)

不同类型的互斥量支持不同的构造方式，比如后面第二个参数用时间点或者时间段的这种构造函数只支持有时限的互斥量。并且不同的第二个参数会导致互斥量处于不同的状态，像adopt_lock和defer_lock就表示互斥量是否是锁定的状态。这个状态可以通过owns_lock函数来获取。如果互斥量满足可锁定要求，则unique_lock亦满足可锁定要求(例如:能用于std::lock);如果互斥量满足可定时锁定要求，则unique_lock满足可定时锁定要求。  
使用例子:
```cpp
#include <mutex>
#include <thread>
#include <chrono>
 
struct Box {
    explicit Box(int num) : num_things{num} {}
 
    int num_things;
    std::mutex m;
};
 
void transfer(Box &from, Box &to, int num)
{
    // 仍未实际取锁
    std::unique_lock<std::mutex> lock1(from.m, std::defer_lock);
    std::unique_lock<std::mutex> lock2(to.m, std::defer_lock);
 
    // 锁两个 unique_lock 而不死锁，这里的lock1和lock2满足可锁定要求，所以可以用于std::lock，因为std::mutex是可锁定的
    std::lock(lock1, lock2);
 
    from.num_things -= num;
    to.num_things += num;
 
    // 'from.m' 与 'to.m' 互斥解锁于 'unique_lock' 析构函数
}
 
int main()
{
    Box acc1(100);
    Box acc2(50);
 
    std::thread t1(transfer, std::ref(acc1), std::ref(acc2), 10);
    std::thread t2(transfer, std::ref(acc2), std::ref(acc1), 5);
 
    t1.join();
    t2.join();
}
```

std::unique_lock和std::lock_guard比较:  
std::unique_lock比std::lock_guard灵活，std::unique_lock比std::lock_guard对象要大，因为他不光存储互斥量，还需要存储互斥量是否已经被锁上。如果std::lock_guard够用就使用std::lock_guard，否则就使用std::unique_lock。而且std::unique_lock是可以移动的，所以就可以将锁的所有权从一个域转移到另外一个域(比如允许一个函数去锁定互斥量，并且将所有权转移到调用者身上)。如下所示:  
```cpp
std::unique_lock<std::mutex> get_lock() {
    extern std::mutex some_mutex;
    std::unique_lock<std::mutex> lk(some_mutex);
    prepare_data();
    return lk;
}
void process_data() {
    std::unique_lock<std::mutex> lk(get_lock());
    do_something();
}
```

### shared_lock(C++14)
```cpp
template <class _Mutex>
class shared_lock
{
public:
    typedef _Mutex mutex_type;

private:
    mutex_type* __m_;
    bool __owns_;

public:
    _LIBCPP_INLINE_VISIBILITY
    shared_lock() _NOEXCEPT
        : __m_(nullptr),
          __owns_(false)
        {}

    _LIBCPP_INLINE_VISIBILITY
    explicit shared_lock(mutex_type& __m)
        : __m_(_VSTD::addressof(__m)),
          __owns_(true)
        {__m_->lock_shared();}

    _LIBCPP_INLINE_VISIBILITY
    shared_lock(mutex_type& __m, defer_lock_t) _NOEXCEPT
        : __m_(_VSTD::addressof(__m)),
          __owns_(false)
        {}

    _LIBCPP_INLINE_VISIBILITY
    shared_lock(mutex_type& __m, try_to_lock_t)
        : __m_(_VSTD::addressof(__m)),
          __owns_(__m.try_lock_shared())
        {}

    _LIBCPP_INLINE_VISIBILITY
    shared_lock(mutex_type& __m, adopt_lock_t)
        : __m_(_VSTD::addressof(__m)),
          __owns_(true)
        {}

    template <class _Clock, class _Duration>
        _LIBCPP_INLINE_VISIBILITY
        shared_lock(mutex_type& __m,
                    const chrono::time_point<_Clock, _Duration>& __abs_time)
            : __m_(_VSTD::addressof(__m)),
              __owns_(__m.try_lock_shared_until(__abs_time))
            {}

    template <class _Rep, class _Period>
        _LIBCPP_INLINE_VISIBILITY
        shared_lock(mutex_type& __m,
                    const chrono::duration<_Rep, _Period>& __rel_time)
            : __m_(_VSTD::addressof(__m)),
              __owns_(__m.try_lock_shared_for(__rel_time))
            {}

    _LIBCPP_INLINE_VISIBILITY
    ~shared_lock()
    {
        if (__owns_)
            __m_->unlock_shared();
    }

    shared_lock(shared_lock const&) = delete;
    shared_lock& operator=(shared_lock const&) = delete;

    _LIBCPP_INLINE_VISIBILITY
    shared_lock(shared_lock&& __u) _NOEXCEPT
        : __m_(__u.__m_),
          __owns_(__u.__owns_)
        {
            __u.__m_ = nullptr;
            __u.__owns_ = false;
        }

    _LIBCPP_INLINE_VISIBILITY
    shared_lock& operator=(shared_lock&& __u) _NOEXCEPT
    {
        if (__owns_)
            __m_->unlock_shared();
        __m_ = nullptr;
        __owns_ = false;
        __m_ = __u.__m_;
        __owns_ = __u.__owns_;
        __u.__m_ = nullptr;
        __u.__owns_ = false;
        return *this;
    }

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    // Setters
    _LIBCPP_INLINE_VISIBILITY
    void swap(shared_lock& __u) _NOEXCEPT
    {
        _VSTD::swap(__m_, __u.__m_);
        _VSTD::swap(__owns_, __u.__owns_);
    }

    _LIBCPP_INLINE_VISIBILITY
    mutex_type* release() _NOEXCEPT
    {
        mutex_type* __m = __m_;
        __m_ = nullptr;
        __owns_ = false;
        return __m;
    }

    // Getters
    _LIBCPP_INLINE_VISIBILITY
    bool owns_lock() const _NOEXCEPT {return __owns_;}

    _LIBCPP_INLINE_VISIBILITY
    explicit operator bool () const _NOEXCEPT {return __owns_;}

    _LIBCPP_INLINE_VISIBILITY
    mutex_type* mutex() const _NOEXCEPT {return __m_;}
};
template <class _Mutex>
void
shared_lock<_Mutex>::lock()
{
    if (__m_ == nullptr)
        __throw_system_error(EPERM, "shared_lock::lock: references null mutex");
    if (__owns_)
        __throw_system_error(EDEADLK, "shared_lock::lock: already locked");
    __m_->lock_shared();
    __owns_ = true;
}

template <class _Mutex>
bool
shared_lock<_Mutex>::try_lock()
{
    if (__m_ == nullptr)
        __throw_system_error(EPERM, "shared_lock::try_lock: references null mutex");
    if (__owns_)
        __throw_system_error(EDEADLK, "shared_lock::try_lock: already locked");
    __owns_ = __m_->try_lock_shared();
    return __owns_;
}

template <class _Mutex>
template <class _Rep, class _Period>
bool
shared_lock<_Mutex>::try_lock_for(const chrono::duration<_Rep, _Period>& __d)
{
    if (__m_ == nullptr)
        __throw_system_error(EPERM, "shared_lock::try_lock_for: references null mutex");
    if (__owns_)
        __throw_system_error(EDEADLK, "shared_lock::try_lock_for: already locked");
    __owns_ = __m_->try_lock_shared_for(__d);
    return __owns_;
}

template <class _Mutex>
template <class _Clock, class _Duration>
bool
shared_lock<_Mutex>::try_lock_until(const chrono::time_point<_Clock, _Duration>& __t)
{
    if (__m_ == nullptr)
        __throw_system_error(EPERM, "shared_lock::try_lock_until: references null mutex");
    if (__owns_)
        __throw_system_error(EDEADLK, "shared_lock::try_lock_until: already locked");
    __owns_ = __m_->try_lock_shared_until(__t);
    return __owns_;
}

template <class _Mutex>
void
shared_lock<_Mutex>::unlock()
{
    if (!__owns_)
        __throw_system_error(EPERM, "shared_lock::unlock: not locked");
    __m_->unlock_shared();
    __owns_ = false;
}

```
从成员函数看出来shared_lock可以支持的互斥量类型:
- lock/try_lock/unlock:内部调用的是lock_shared，try_lock_shared, unlock_shared
- try_lock_for/try_lock_until: 内部调用的是try_lock_shared_for，try_lock_shared_until  
这些接口全部要求共享式加锁解锁，因此，shared_lock仅适用于shared_mutex，shared_timed_mutex这两种支持共享式加锁的mutex。同时类似于unique_lock，shared_lock也支持构造的时候指定锁的状态，是否构造的时候就加锁，并且shared_lock支持移动操作，也能转移锁的所有权。
# condition_variable相关
### std::condition_variable
条件变量std::condition_variable(以下简称cv)也是一种用来同步的原语。可以阻塞多个线程，直到一个线程修改了条件，并且通知其他线程。cv需要结合互斥量一起使用，cv的每个wait函数第一个参数都是unique_lock<mutex>。cv只是为了让多个线程间同步协作，这对于生产者-消费者模型很有意义。在这个模型下:
- 生产者和消费者共享一个工作区，这个工作区的大小是有限的。
- 生产者总是生成数据放到工作区中，当工作区满了，它就停下来等消费者消费一部分数据，然后继续工作。
- 消费者总是从工作区中拿出数据使用，当工作区中的数据被消费空了之后，它也会停下来等待生产者往工作区中放入新的数据。
cv的作用只是让线程正常扮演生产者消费者的角色，消费者线程往工作区里面放数据，消费者线程从工作区里面拿数据。这里的工作区就是临界区，但是为了保证消费者线程能够互斥的操作工作区里面的数据，所以才需要配合mutex使用。
```cpp
class _LIBCPP_TYPE_VIS condition_variable
{
    __libcpp_condvar_t __cv_ = _LIBCPP_CONDVAR_INITIALIZER;
public:
    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR condition_variable() _NOEXCEPT = default;

#ifdef _LIBCPP_HAS_TRIVIAL_CONDVAR_DESTRUCTION
    ~condition_variable() = default;
#else
    ~condition_variable();
#endif

    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;

    void notify_one() _NOEXCEPT;
    void notify_all() _NOEXCEPT;

    void wait(unique_lock<mutex>& __lk) _NOEXCEPT;
    //等待唤醒，被唤醒后如果满足__pred,就去锁定__lk
    template <class _Predicate>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        void wait(unique_lock<mutex>& __lk, _Predicate __pred);

    template <class _Clock, class _Duration>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        cv_status
        wait_until(unique_lock<mutex>& __lk,
                   const chrono::time_point<_Clock, _Duration>& __t);

    template <class _Clock, class _Duration, class _Predicate>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        bool
        wait_until(unique_lock<mutex>& __lk,
                   const chrono::time_point<_Clock, _Duration>& __t,
                   _Predicate __pred);

    template <class _Rep, class _Period>
        _LIBCPP_METHOD_TEMPLATE_IMPLICIT_INSTANTIATION_VIS
        cv_status
        wait_for(unique_lock<mutex>& __lk,
                 const chrono::duration<_Rep, _Period>& __d);

    template <class _Rep, class _Period, class _Predicate>
        bool
        _LIBCPP_INLINE_VISIBILITY
        wait_for(unique_lock<mutex>& __lk,
                 const chrono::duration<_Rep, _Period>& __d,
                 _Predicate __pred);

    typedef __libcpp_condvar_t* native_handle_type;
    _LIBCPP_INLINE_VISIBILITY native_handle_type native_handle() {return &__cv_;}

private:
    void __do_timed_wait(unique_lock<mutex>& __lk,
       chrono::time_point<chrono::system_clock, chrono::nanoseconds>) _NOEXCEPT;
#if defined(_LIBCPP_HAS_COND_CLOCKWAIT)
    void __do_timed_wait(unique_lock<mutex>& __lk,
       chrono::time_point<chrono::steady_clock, chrono::nanoseconds>) _NOEXCEPT;
#endif
    template <class _Clock>
    void __do_timed_wait(unique_lock<mutex>& __lk,
       chrono::time_point<_Clock, chrono::nanoseconds>) _NOEXCEPT;
};
```
注: cv不可拷贝构造，不可拷贝赋值  
cv提供了两类操作:wait和notify分别对应消费者和生产者，wait就是消费者在等待工作区中的数据，notify就是生产者生成了数据通知消费者来消费。
- wait:
```cpp
template <class _Predicate>
void condition_variable::wait(unique_lock<mutex>& __lk, _Predicate __pred)
{
    while (!__pred())
        wait(__lk);
}

void
condition_variable::wait(unique_lock<mutex>& lk) _NOEXCEPT
{
    if (!lk.owns_lock())
        __throw_system_error(EPERM,
                                  "condition_variable::wait: mutex not locked");
    int ec = __libcpp_condvar_wait(&__cv_, lk.mutex()->native_handle());
    if (ec)
        __throw_system_error(ec, "condition_variable wait failed");
}

int __libcpp_condvar_wait(__libcpp_condvar_t *__cv, __libcpp_mutex_t *__m)
{
  return pthread_cond_wait(__cv, __m);
}
```
生产者线程通过notify来唤醒正在wait的消费者线程，唤醒后就去获取__lk，获取失败会继续等待。获取之后就去检查条件是否满足，如果不满足，自动释放mutex，然后线程继续阻塞在wait上面。如果条件满足就返回并继续持有锁继续往下执行。至于为什么不使用std::lock_guard而是使用std::unique_lock，因为这里如果不满足条件要解锁，而lock_guard没有这么灵活。wait最终会调用操作系统级别的pthread_cond_wait去等待这个unique_lock。
注意:  
wait有时候会在没有任何线程调用notify的情况下返回，这种情况就是有名的spurious wakeup(伪唤醒)。因此当wait返回时，你需要再次检查wait的前置条件是否满足，如果不满足则需要再次wait。wait提供了重载的版本，用于提供前置检查。本质上来说cv::wait是对“忙碌-等待”的一种优化，不理想的方式就是用简单的循环来实现
```cpp
template<typename Predicate>
void minimal_wait(std::unique_lock<std::mutex>& lk,Predicate
pred){
    while(!pred()){
        lk.unlock();
        lk.lock();
    }
}

```
wait还有wait_for和wait_util版本，类似带有超时的mutex，一个指定时间段，一个指定时间点，只在这个时间段内进行wait，超出时间后就返回超时。
- notify
notify就比wait要简单，notify的作用就是用来唤醒wait在cv上的线程。notify有两个版本：  
notify_one唤醒等待的一个线程，注意只唤醒一个。notify_all唤醒所有wait在cv上的线程。

使用例子:
```cpp
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
 
std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;
 
void worker_thread()
{
    // 等待直至 main() 发送数据
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, []{return ready;});
 
    // 等待后，我们占有锁。
    std::cout << "Worker thread is processing data\n";
    data += " after processing";
 
    // 发送数据回 main()
    processed = true;
    std::cout << "Worker thread signals data processing completed\n";
 
    // 通知前完成手动解锁，以避免等待线程才被唤醒就阻塞（细节见 notify_one ）
    lk.unlock();
    cv.notify_one();
}
 
int main()
{
    std::thread worker(worker_thread);
 
    data = "Example data";
    // 发送数据到 worker 线程
    {
        std::lock_guard<std::mutex> lk(m);
        ready = true;
        std::cout << "main() signals data ready for processing\n";
    }
    cv.notify_one();
 
    // 等候 worker
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, []{return processed;});
    }
    std::cout << "Back in main(), data = " << data << '\n';
 
    worker.join();
}
```