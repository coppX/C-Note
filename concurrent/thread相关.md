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