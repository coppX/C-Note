## push_back和emplace_back的区别
push_back和emplace_back都是往vector(deque和list也有这两个函数)尾部插入新的元素，emplace_back是C++11新增的接口，先看看vector中他们的实现
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
push_back有两个版本，分为const _Tp&和_Tp&&作为参数类型的版本，其中_Tp&&这个版本是C++11才有的；  
emplace_back也有两个版本，分为返回值类型为void和_Tp&，其中返回void的是C++11新增的，返回_Tp&的版本是C++14新增的；
注意:从C++20开始，上面的版本都废弃了，改成上面的版本前面加上constexpr关键字(目前我手上的libcxx源代码还没实现对新接口的支持)。  
看看push_back的实现
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
接下来是emplace_back的实现
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

看看统一在空间足够的情况下的操作__construct_one_at_end
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
这里的__alloc_traits::construct参照std::allocator_traits<Alloc>::construct的定义
```cc
template<class T, class... Args>
static void construct(Alloc& a, T* p, Args&&... args);
```
其中__alloc_traits即为allocator_traits<_Allocator>，其中_Allocator如果在vector<T>定义的时候没有声明就默认为std::allocator<T>，所以此时的__alloc_traits可以看成allocator_traits<allocator<T>>，construct函数也可以看成construct(allocator<T>& a, T* p, Args&&.. args)。construct定义如下 
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

因此，上面在push_back和emplace_back调用__construct_one_at_end的时候，传递的参数就会影响到这个元素的构造，首先push_back传递的类型是const _Tp&和_Tp&&，会调用construct里面的第一个函数把参数放置在vector最后的位置,而emplace_back则会调用construct下面这个函数先构造一个_Tp类型的对象，然后再给放到vector最后的位置。至于判断调用的是上面的函数还是下面的函数，通过__has_construct结构体来判断的。总结就是emplace_back可以直接传一个参数，先用这个参数构造容器元素类型，构造完就直接给加在容器的最后面，而push_back想要将一个容器元素类型的变量放到容器后面，需要通过拷贝这个变量或者移动这个变量到容器最后面。