C++中为了防止内存泄露，除了RAII，还可以使用智能指针。
# shared_ptr
shared_ptr是通过指针保持对象共享所有权的智能指针，多个shared_ptr对象可以指向同一对象。shared_ptr的部分定义如下:
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
看看shared_ptr具体的一个构造函数的定义:
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
        //这里引用数量减为0了就析构掉对象
        __on_zero_shared();
        return true;
      }
      return false;
    }
    ...
};
```
可以看到关于`share_ptr`里面的共享引用数`shared_count`部分是通过原子操作来实现的自增自减，多个线程里面的`share_ptr`也不会对`shared_count`造成线程安全问题。但是，`shared_ptr`里面的`__ptr_`是直接赋的值，如果多个线程中的`shared_ptr`指向同一个对象，其中一个线程对 `__ptr_`指向的对象加一，另外一个线程对`__ptr_`指向的对象减一，那么这个时候就会出现问题了，结果可能就不确定了。所以常规做法是在写shared_ptr的时候进行加锁操作，例如:
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
UE实现的TSharedPtr是’有条件‘的线程安全的，
```cc
template< class ObjectType, ESPMode Mode >
class TSharedPtr
{
public:
	using ElementType = ObjectType;

    template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TSharedPtr( OtherType* InObject )
		: Object( InObject )
		, SharedReferenceCount( SharedPointerInternals::NewDefaultReferenceController( InObject ) )
	{
		UE_TSHAREDPTR_STATIC_ASSERT_VALID_MODE(ObjectType, Mode)

		SharedPointerInternals::EnableSharedFromThis( this, InObject, InObject );
	}
};
```
模板TSharedPtr第二个模板参数是有三个选择的，线程不安全，线程安全和快速模式，默认选择是Fast
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
TSharedPtr对于线程安全的检查关键在于UE_TSHAREDPTR_STATIC_ASSERT_VALID_MODE(ObjectType, Mode)，实现如下:
```cc
#define UE_TSHAREDPTR_STATIC_ASSERT_VALID_MODE(ObjectType, Mode) \
	enum \
	{ \
		ObjectTypeHasSameModeSharedFromThis = TPointerIsConvertibleFromTo<ObjectType, TSharedFromThis<ObjectType, Mode>>::Value, \
		ObjectTypeHasOppositeModeSharedFromThis = TPointerIsConvertibleFromTo<ObjectType, TSharedFromThis<ObjectType, (Mode == ESPMode::NotThreadSafe) ? ESPMode::ThreadSafe : ESPMode::NotThreadSafe>>::Value \
	}; \
	static_assert(ObjectTypeHasSameModeSharedFromThis || !ObjectTypeHasOppositeModeSharedFromThis, "You cannot use a TSharedPtr of one mode with a type which inherits TSharedFromThis of another mode.");

```
上述代码核心在于ObjectType和TSharedFromThis<ObjectType, Mode>>::Value是否能否相互转换，来看看TSharedFromThis在UE里面的实现:
```cc
template< class ObjectType, ESPMode Mode >
class TSharedFromThis
{
    ...
private:
    mutable TWeakPtr< ObjectType, Mode > WeakThis;
};
```
这里的TSharedFromThis里面确实也类似于STL里面的一样利用的TWeakPtr来实现一系列功能。
```cc
template< class ObjectType, ESPMode Mode >
class TWeakPtr
{
private:
	ObjectType* Object;

	SharedPointerInternals::FWeakReferencer< Mode > WeakReferenceCount;
};
```
也只有WeakReferenceCount使用了Mode参数,ObjectType*根本就没有用到任何线程安全相关的操作。所以TSharedPtr也只是对引用计数是线程安全的，对于他引用的对象还是线程不安全的。  
接着看FWeakReferencer的实现
```cc
template< ESPMode Mode >
class FSharedReferencer
{
	typedef FReferenceControllerOps<Mode> TOps;
    ...
};
```
其中FReferenceControllerOps:
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


### 题外话enable_shared_from_this
STL里面也有std::enable_shared_from_this,他的实现原理是内部有一个weak_ptr类型的成员变量_Wptr,当shared_ptr构造的时候，如果其模板类型继承了enable_shared_from_this，则对_Wptr进行初始化操作，这样将来调用shared_from_this函数的时候，就能通过weak_ptr构造出来对应的shared_ptr。  
# 参考文献
https://zhuanlan.zhihu.com/p/348650382
