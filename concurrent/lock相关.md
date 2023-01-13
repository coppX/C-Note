## std::lock相关
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

### std::shared_lock(C++14)
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