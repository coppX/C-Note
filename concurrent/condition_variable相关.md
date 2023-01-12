## std::condition_variable
条件变量std::condition_variable(以下简称cv)也是一种用来同步的原语。可以阻塞多个线程，直到一个线程修改了条件，并且通知其他线程。cv需要结合互斥量一起使用，cv的每个wait函数第一个参数都是unique_lock<mutex>。cv只是为了让多个线程间同步协作，这对于生产者-消费者模型很有意义。在这个模型下:
- 生产者和消费者共享一个工作区，这个工作区的大小是有限的。
- 生产者总是生成数据放到工作区中，当工作区慢了，它就停下来等消费者消费一部分数据，然后继续工作。
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
```
生产者线程通过notify来唤醒正在wait的消费者线程，唤醒后就去获取__lk，获取失败会继续等待。获取之后就去检查条件是否满足，如果不满足，自动释放mutex，然后线程继续阻塞在wait上面。如果条件满足就返回并继续持有锁继续往下执行。至于为什么不使用std::lock_guard而是使用std::unique_lock，因为这里如果不满足条件要解锁，而lock_guard没有这么灵活。
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