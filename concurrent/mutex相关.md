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