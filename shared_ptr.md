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

		// If the object happens to be derived from TSharedFromThis, the following method
		// will prime the object with a weak pointer to itself.
		SharedPointerInternals::EnableSharedFromThis( this, InObject, InObject );
	}
};
```