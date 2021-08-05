# shared_ptr的线程安全性
`shared_ptr`是通过指针保持对象共享所有权的智能指针，多个`shared_ptr`对象可以指向同一对象。`shared_ptr`的部分定义如下:
```cc
template<class _Tp>
class _LIBCPP_TEMPLATE_VIS shared_ptr
{
public:
    typedef _Tp element_type;
private:
    element_type*      __ptr_;
    __shared_weak_count* __cntrl_;
    ...
};
```
看看`shared_ptr`具体的一个构造函数的定义:
```cc
//普通构造
template<class _Tp>
template<class _Yp>
shared_ptr<_Tp>::shared_ptr(_Yp* __p,
                            typename enable_if<is_convertible<_Yp*, element_type*>::value, __nat>::type)
    : __ptr_(__p)
{
    unique_ptr<_Yp> __hold(__p);
    typedef typename __shared_ptr_default_allocator<_Yp>::type _AllocT;
    typedef __shared_ptr_pointer<_Yp*, default_delete<_Yp>, _AllocT > _CntrlBlk;
    __cntrl_ = new _CntrlBlk(__p, default_delete<_Yp>(), _AllocT());
    __hold.release();
    __enable_weak_this(__p, __p);
}

//拷贝构造
template<class _Tp>
template<class _Yp>
inline
shared_ptr<_Tp>::shared_ptr(const shared_ptr<_Yp>& __r, element_type *__p) _NOEXCEPT
    : __ptr_(__p),
      __cntrl_(__r.__cntrl_)
{
    //这里会add_shared();
    if (__cntrl_)
        __cntrl_->__add_shared();
}

```
上面的`__add_shared()`是由`__cntrl`调用，也就是`__shared_weak_count`类型调用`__add_shared()`,以下是`__shared_weak_count::__add_shared`的实现:
```cc
class _LIBCPP_TYPE_VIS __shared_weak_count
    : private __shared_count
{
    ...
    _LIBCPP_INLINE_VISIBILITY
    void __add_shared() _NOEXCEPT {
      __shared_count::__add_shared();
    }
    
    _LIBCPP_INLINE_VISIBILITY
    void __release_shared() _NOEXCEPT {
      if (__shared_count::__release_shared())
        __release_weak();
    }
    ...
};
```
其中`__shared_count::__add_shared()`的实现如下:
```cc
class _LIBCPP_TYPE_VIS __shared_count
{
    ...
        _LIBCPP_INLINE_VISIBILITY
    void __add_shared() _NOEXCEPT {
        //这里是用的原子操作，递增引用
      __libcpp_atomic_refcount_increment(__shared_owners_);
    }
    _LIBCPP_INLINE_VISIBILITY
    bool __release_shared() _NOEXCEPT {
        ////这里是用的原子操作，递减引用
      if (__libcpp_atomic_refcount_decrement(__shared_owners_) == -1) {
        //这里引用数量减为0了就析构掉对象,没看懂为啥==-1
        __on_zero_shared();
        return true;
      }
      return false;
    }
    ...
};
```
可以看到关于`share_ptr`里面的共享引用数`shared_count`部分是通过原子操作来实现的自增自减，多个线程里面的`share_ptr`也不会对`shared_count`造成线程安全问题。
## 什么是原子操作
原子操作是个不可分割的操作。系统的所有线程中，不可能观察到原子操作完成了一半。如果读取对象的加载操作是原子的，那么这个对象的所有修改操作也是原子的，所以加载操作得到的值要么是对象的初始值，要么是某次修改操作存入的值。

另一方面，非原子操作可能会被另一个线程观察到只完成一半。如果这个操作是一个存储操作，那么其他线程看到的值，可能既不是存储前的值，也不是存储的值。如果非原子操作是一个读取操作，可能先取到对象的一部分，然后值被另一个线程修改，然后它再取到剩余的部分，所以它取到的既不是第一个值，也不是第二个值。这就构成了数据竞争，出现未定义行为。  

在C++标准库中有标准原子类型，这些原子类型的操作都是原子的，但是他们的实现有点不一样，取决于硬件平台，这些原子类型都提供一个`is_lock_free()`的成员函数，这个函数可以用来判断这个原子类型是直接用的原子指令(`x.is_lock_free()`返回`true`)还是内部用的锁结构(`x.is_lock_free()`返回`false`)来实现的操作的原子性。 
## 为什么shared_ptr指向的对象不是线程安全的
举个例子，如果多个线程中的`shared_ptr`指向同一个`int`类型的对象，其中一个线程对 `__ptr_`指向的对象加一，同一时间另外一个线程对`__ptr_`指向的对象减一，那么这个时候就会出现问题了，结果到底是加了1还是减了1还是不变就不确定了。  
关于线程安全，《Java并发编程实战》给出的定义如下：一个对象是否需要是线程安全的，取决于它是否被多个线程访问。这只和对象在程序中是以何种方式被使用的有关，和对象本身具体是做什么的无关。当多个线程访问某个类时，这个类始终都能表现出正确的行为，那么就称这个类是线程安全的。线程安全的程序不一定是由线程安全的类组成，完全由线程安全类组成的程序也不一定是线程安全的。还需要一定的组合技巧才能保证线程安全。
## 线程安全地使用shared_ptr
为了线程安全地使用`shared_ptr`,所以常规做法是在写`shared_ptr`的时候进行加锁操作，例如:
```cc
void thr(std::shared_ptr<Base> p)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::shared_ptr<Base> lp = p; // 线程安全，虽然自增共享的 use_count
    {
        static std::mutex io_mutex;
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << "local pointer in a thread:\n"
                  << "  lp.get() = " << lp.get()
                  << ", lp.use_count() = " << lp.use_count() << '\n';
    }
}
```

# UE实现的TSharedPtr
听说，UE也实现了`TSharedPtr`，当我翻开他代码的时候，他注释里面写的是`TSharedPtr`是’有条件‘的线程安全的，so
```cc
template< class ObjectType, ESPMode Mode >
class TSharedPtr
{
public:
	using ElementType = ObjectType;
    ...
    FORCEINLINE TSharedPtr( TSharedPtr< OtherType, Mode > const& InSharedPtr )
		: Object( InSharedPtr.Object )
		, SharedReferenceCount( InSharedPtr.SharedReferenceCount )
	{
	}

	FORCEINLINE TSharedPtr( TSharedPtr const& InSharedPtr )
		: Object( InSharedPtr.Object )
		, SharedReferenceCount( InSharedPtr.SharedReferenceCount )
	{
	}
    ...
private:
	ObjectType* Object;
	SharedPointerInternals::FSharedReferencer< Mode > SharedReferenceCount;
};
```
模板`TSharedPtr`第二个模板参数是有三个选择的，线程不安全，线程安全和快速模式，默认选择是`Fast`
```cc
enum class ESPMode
{
	/** Forced to be not thread-safe. */
	NotThreadSafe = 0,

	/**
		*	Fast, doesn't ever use atomic interlocks.
		*	Some code requires that all shared pointers are thread-safe.
		*	It's better to change it here, instead of replacing ESPMode::Fast to ESPMode::ThreadSafe throughout the code.
		*/
	Fast = FORCE_THREADSAFE_SHAREDPTRS ? 1 : 0,

	/** Conditionally thread-safe, never spin locks, but slower */
	ThreadSafe = 1
};
```
当我们用一个`TSharedPtr`来拷贝构造另外一个`TSharedPtr`时，他的拷贝构造操作直接把指向对象的指针给赋值了，然后把引用计数也赋值了，他这个引用计数与线程安全参数有关，裸指针并没有线程方面的属性修饰
  
接着看`FSharedReferencer`的实现
```cc
template< ESPMode Mode >
class FSharedReferencer
{
    FORCEINLINE FSharedReferencer( FSharedReferencer const& InSharedReference )
        : ReferenceController( InSharedReference.ReferenceController )
    {
        // If the incoming reference had an object associated with it, then go ahead and increment the
        // shared reference count
        if( ReferenceController != nullptr )
        {
            TOps::AddSharedReference(ReferenceController);
        }
    }

	typedef FReferenceControllerOps<Mode> TOps;
    ...
};
```
只要拷贝构造这个引用计数，他里面会执行增加引用计数的操作，这个操作由`TOps`来实现，其中`FReferenceControllerOps`定义为:
```cc
//'线程安全'版本
template<>
struct FReferenceControllerOps<ESPMode::ThreadSafe>
{   
    //这里对引用计数还是利用的原子操作来实现自增的
    static FORCEINLINE void AddSharedReference(FReferenceControllerBase* ReferenceController)
    {
        FPlatformAtomics::InterlockedIncrement( &ReferenceController->SharedReferenceCount );
    }
};
//非线程安全版本
template<>
struct FReferenceControllerOps<ESPMode::NotThreadSafe>
{
    //引用计数直接++
    static FORCEINLINE void AddSharedReference(FReferenceControllerBase* ReferenceController) TSAN_SAFE_UNSAFEPTR
    {
        ++ReferenceController->SharedReferenceCount;
    }
}
```
所以，关于`TSharedPtr`，分为了两个版本，线程安全和线程不安全的，其中线程安全版本针对了引用计数进行了原子操作来进行增减。但是，对于`TSharedPtr`所指向的对象的访问没有任何的线程方面的限制。  
综上所述，UE里面的`TSharedPtr`也是个线程不安全的实现！他所声称的部分线程安全也仅仅是说引用计数是线程安全的，对于所指向的对象却不是。

## 题外话  enable_shared_from_this
`enable_shared_from_this`也能产生一个`shared_ptr`，但是这既然标准库已经有了`shared_ptr`，那么`enable_shared_from_this`又有什么用呢，考虑下面一种情况:
```cc
void sock_sender::post_request_no_lock()
{
    Request &req = requests_.front();
    boost::asio::async_write(
        *sock_ptr_,
        boost::asio::buffer(req.buf_ptr->get_content()),
        bind(&sock_sender::self_handler, this, _1, _2));
}
```
异步编程时，我们在传入回调函数的时候，通常需要带上当前类的上下文，或者回调本身就是回调函数，那么这个工作非`this`莫属了，但是这里可能会出事，如果执行异步回调的时候`this`已经销毁了怎么办，程序直接crash。这个时候就能用到`enable_shared_from_this`来解决这个问题。
```cc
class sock_sender : public enable_shared_from_this
{
    //...
};
void sock_sender::post_request_no_lock()
{
    Request &req = requests_.front();
    boost::asio::async_write(
        *sock_ptr_,
        boost::asio::buffer(req.buf_ptr->get_content()),
        bind(&sock_sender::self_handler, shared_from_this(), _1, _2));
}
```
ok,这里`shared_from_this()`直接给`sock_sender`对象续了一命，直到这个异步回调完成后才会销毁掉`sock_sender`对象。  
那有人问了，这里为什么不能直接使用`shared_ptr<sock_sender>(this)`来代替`shared_from_this()`，这个当然不可以,你想想，如果这个对象是通过`shared_ptr<sock_sender> p(new sock_sender); `定义的怎么办，这种情况下，这两个`shared_ptr`都是独自调用普通的构造函数，不是拷贝构造函数来由其中一个构造另外一个，按照上面的`shared_ptr`源代码就可以看出来他们的引用计数不是叠加在一起的，如果`p`给结束了生命周期会释放掉这个对象，异步函数执行的时候还是会crash。这就类似于:
```cc
T* t = new T();
shared_ptr<T> t1(t);
shared_ptr<t> t2(t);
```
是一样的道理,这里的`t1`和`t2`会导致`t`被释放两次，程序直接炸裂。

# 参考文献
- [《C++ Concurrency in Action 2nd》](https://github.com/xiaoweiChen/CPP-Concurrency-In-Action-2ed-2019/blob/master/content/chapter5/5.2-chinese.md)  
- 《Linux多线程服务端编程》
- [llvm-mirror/libcxx/shared_ptr](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/memory#L3651)
- [UE中TSharedPtr实现](https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Core/Public/Templates/SharedPointer.h#L559)
- https://zh.cppreference.com/w/cpp/memory/shared_ptr  
- https://zhuanlan.zhihu.com/p/348650382
- https://www.zhihu.com/question/30957800/answer/50181754
- https://bbs.huaweicloud.com/blogs/136194
