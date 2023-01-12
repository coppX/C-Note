## std::mutex相关
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
这里的lock定义后面，有个acquire_capability,包括下面的try_acquire_capability和release_capability，这是clang编译器的特性，是clang的线程安全静态分析选项结合使用的代码注释, [详情参考](https://clang.llvm.org/docs/ThreadSafetyAnalysis.html)
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
这里同样类似于mutex用了Clang的线程安全分析选项。[详情查看](https://clang.llvm.org/docs/ThreadSafetyAnalysis.html)  
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
第一次调用lock，最开始__state_为0，__write_entered_为0x80000000，不会进入第一个while， __state_变成0x80000000，而__n_readers_为0x7fffffff，同样不会进入第二个循环, 第一次lock是不会进行wait操作的, 条件变量__gate1_和__gate2_都没有被启用。如果这个时候没有调用unlock，__state_就不会被重置，继续调用lock，那么就会进入第一个while，表示已经进入了独占锁的状态，__gate1.wait就会阻塞在这里等待，除非调用一次unlock来对__gate1_调用notify_all进行唤醒，也就是释放掉独占锁才能重新再次加锁。这里如果能进入第二个，表示shared_mutex已经是处于共享锁的状态了，就应该把这里的共享锁__gate2给阻塞起来，除非在unlock_shared里面把所有的读锁都释放掉，才能唤醒__gate2_。在加上了独占锁(__state_ != 0)的情况下，__gate1和__gate2都会阻塞在这里。 
try_lock就是如果__state == 0的情况下就表示没有加锁，可以进行上锁操作，参考上面描述的第一次locK，__state为0，否则就加锁失败。  
unlock重置掉__state_，并且唤醒阻塞的__gate1_。  
__gate1_表示是的独占状态下对加锁的阻塞和记录读锁数量满了的情况下的阻塞，__gate2_表示的是共享状态下对加锁的阻塞，__state_用来记录是否处于独占锁，和有多少共享锁。   
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
首先看lock_shared，while的判断条件中如果__state & __write_entered_ != 0就表示已经加上了独占锁，需要阻塞在__gate1._上，或者(__state_ & __n_readers_) == __n_readers_的情况下也会阻塞在__gate1_上(读的数量已经达到最大了，不能再加了，记录不下了)。等shared_mutex调用unlock释放独占锁来唤醒__gate1_或者调用unlock_shared来唤醒__gate1_。其他情况，只需要增加reader的数量就ok，并记录读的数量到__state_中。  
try_lock_shared类似于lock_shared的判断条件，判断一下是否处于非独占状态，并且共享锁的数量没有记录满的情况下，就可以对其进行try_lock_shared，否则就不行。  
unlock_shared首先会减少记录的读的数量，进入__state_ & __write_entered_判断后，表示目前处于独占锁的状态，那么这个时候调用unlock_shared可以对lock里面阻塞的__gate2_进行唤醒。否则，如果num_readers == __n_readers_ - 1就表示释放掉一个共享锁后，可以记录新的共享锁，就可以唤醒lock_shared里面阻塞的_gate1_。类似于消费者消费掉了一个数量后，我们唤醒生产者继续生产。

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
第二个if表示我们在有读线程的情况下，进行独占加锁的操作，那么我们就会对__gate2_进行阻塞等待，如果某次__gate2_醒来后发现，(__base.__state_ & __base.__n_readers_) == 0成立，也就是说没有读线程了，只有写线程，那么我们就不需要对__gate2_进行等待操作了，直接break, 返回true，独占加锁成功， 已经没有共享加锁了。如果等待的结果是timeout，那么已经等待超时了，我们取消掉独占加锁(__base.__state_ &= ~__base.__write_entered_)，并且尝试唤醒__gate1_，返回false，加锁失败。
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