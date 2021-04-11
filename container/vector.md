## push_back和emplace_back的区别
`push_back`和`emplace_back`都是往`vector`(`deque`和`list`也有这两个函数)尾部插入新的元素，`emplace_back`是C++11新增的接口，先看看`vector`中他们的实现
```cc
template<class _Tp, class _Allocator /* = allocator<_Tp> */>
class _LIBCPP_TEMPLATE_VIS vector : private __vector_base<_Tp, _Allocator>
{
private:
    typedef __vector_base<_Tp, _Allocator>           __base;
    typedef allocator<_Tp>                           __default_allocator_type;
public:
    typedef _Tp                                      value_type;
    typedef typename __base::__alloc_traits          __alloc_traits;    //即allocator_traits<_Allocator>
    typedef typename __base::reference               reference;         //即_Tp&
    typedef typename __base::const_reference         const_reference;   //即const _Tp&
    ...
    _LIBCPP_INLINE_VISIBILITY void push_back(const_reference __x);

#ifndef _LIBCPP_CXX03_LANG
    _LIBCPP_INLINE_VISIBILITY void push_back(value_type&& __x);
    
    template <class... _Args>
    _LIBCPP_INLINE_VISIBILITY
#if _LIBCPP_STD_VER > 14
        reference emplace_back(_Args&&... __args);
#else
        void      emplace_back(_Args&&... __args);
#endif
    ...
}
```
`push_back`有两个版本，分为`const _Tp&`和`_Tp&&`作为参数类型的版本，其中`_Tp&&`这个版本是C++11才有的；  
`emplace_back`也有两个版本，分为返回值类型为`void`和`_Tp&`，其中返回`void`的是C++11新增的，返回`_Tp&`的版本是C++14新增的;  
注意:从C++20开始，上面的版本都废弃了，改成上面的版本前面加上`constexpr`关键字(目前我手上的libcxx源代码还没实现对新接口的支持)。 
 
看看`push_back`的实现
```cc
template <class _Tp, class _Allocator>
inline _LIBCPP_INLINE_VISIBILITY
void
vector<_Tp, _Allocator>::push_back(const_reference __x)
{
    if (this->__end_ != this->__end_cap())
    {
        __construct_one_at_end(__x);
    }
    else
        __push_back_slow_path(__x);
}

#ifndef _LIBCPP_CXX03_LANG

template <class _Tp, class _Allocator>
inline _LIBCPP_INLINE_VISIBILITY
void
vector<_Tp, _Allocator>::push_back(value_type&& __x)
{
    if (this->__end_ < this->__end_cap())
    {
        __construct_one_at_end(_VSTD::move(__x));
    }
    else
        __push_back_slow_path(_VSTD::move(__x));
}
```
接下来是`emplace_back`的实现
```cc
template <class _Tp, class _Allocator>
template <class... _Args>
inline
#if _LIBCPP_STD_VER > 14
typename vector<_Tp, _Allocator>::reference
#else
void
#endif
vector<_Tp, _Allocator>::emplace_back(_Args&&... __args)
{
    if (this->__end_ < this->__end_cap())
    {
        __construct_one_at_end(_VSTD::forward<_Args>(__args)...);
    }
    else
        __emplace_back_slow_path(_VSTD::forward<_Args>(__args)...);
#if _LIBCPP_STD_VER > 14
    return this->back();
#endif
}
```

看看`push_back`和`emplace_back`在空间足够的情况下的操作`__construct_one_at_end`
```cc
  template <class ..._Args>
  _LIBCPP_INLINE_VISIBILITY
  void __construct_one_at_end(_Args&& ...__args) {
    _ConstructTransaction __tx(*this, 1);
    __alloc_traits::construct(this->__alloc(), _VSTD::__to_raw_pointer(__tx.__pos_),
        _VSTD::forward<_Args>(__args)...);
    ++__tx.__pos_;
  }
```
这里的`__alloc_traits::construct`参照`std::allocator_traits<Alloc>::construct`的定义
```cc
template<class T, class... Args>
static void construct(Alloc& a, T* p, Args&&... args);
```
其中`__alloc_traits`即为`allocator_traits<_Allocator>`，其中`_Allocator`如果在`vector<T>`定义的时候没有声明就默认为`std::allocator<T>`，所以此时的`__alloc_traits`可以看成`allocator_traits<allocator<T>>`，construct函数也可以看成`construct(allocator<T>& a, T* p, Args&&.. args)`。construct定义如下 
```cc
template <class _Tp, class... _Args>
    _LIBCPP_INLINE_VISIBILITY
    static void construct(allocator_type& __a, _Tp* __p, _Args&&... __args)
        {__construct(__has_construct<allocator_type, _Tp*, _Args...>(),
            __a, __p, _VSTD::forward<_Args>(__args)...);}
...
#ifndef _LIBCPP_HAS_NO_VARIADICS
    template <class _Tp, class... _Args>
        _LIBCPP_INLINE_VISIBILITY
        static void __construct(true_type, allocator_type& __a, _Tp* __p, _Args&&... __args)
            {__a.construct(__p, _VSTD::forward<_Args>(__args)...);}
    template <class _Tp, class... _Args>
        _LIBCPP_INLINE_VISIBILITY
        static void __construct(false_type, allocator_type&, _Tp* __p, _Args&&... __args)
            {
                ::new ((void*)__p) _Tp(_VSTD::forward<_Args>(__args)...);
            }
#endif  // _LIBCPP_HAS_NO_VARIADICS
```

因此，上面在`push_back`和`emplace_back`调用`__construct_one_at_end`的时候，传递的参数就会影响到这个元素的构造，首先`push_back`传递的类型是`const _Tp&`和`_Tp&&`，会调用construct里面的第一个函数把参数放置在`vector`最后的位置,而`emplace_back`则会调用`construct`下面这个函数直接在`vector`的最后位置构造一个`_Tp`类型的变量。至于判断调用的是上面的函数还是下面的函数，通过`__has_construct`结构体来判断的。总结就是`emplace_back`可以直接传一个参数，直接就在容器的最后面构造了一个容器类型的变量，而`push_back`想要将一个容器元素类型的变量放到容器后面，需要通过拷贝这个变量或者移动这个变量到容器最后面，所以`emplace_back`从效率上来讲更胜一筹。