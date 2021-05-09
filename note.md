## `std::bind`
`std::bind`的参数传递的时候是按照拷贝或者移动传递的，绝对不会按照引用传递，除非用`std::ref`或`std::cref`包装参数。至于是传拷贝还是传移动是根据这个参数到底是可拷贝还是可移动来决定的。
## `std::ref`和`std::cref`
`std::bind`和`std::thread`在往函数体里面传参数的时候，可能由于`std::bind`和`std::thread`并不清楚参数在执行的时候是否还有效。比如新创建的thread t并对t采用`t.detach()`，如果主线程结束运行了，主线程释放了所有的变量，而t里面的线程函数由于是使用创建时从主线程中传进来的变量，那么就会造成错误。`std::bind`也一样的，`std::bind`只是把参数和一个函数绑定，等到调用这个函数的时候这个参数是否还有效果那就不确定了。所以在`std::bind`和`std::thread`的给函数体传参数的时候是传的这个参数的拷贝/移动而不是引用。`std::bind`和`std::thread`会忽略掉函数期待的参数类型，即使这个函数的形参是引用类型的，还是给他传拷贝/移动。当然如果我们想给这个函数传递引用，那么就采用`std::ref`和`std::cref`  

那么，`std::ref`和`&`的区别是啥？  
`std::ref`实际上返回的是`reference_wrapper`而不是`T&`
`std::ref`生成`std::reference_wrapper`类型对象的帮助函数，用模板实参推导确定结果的模板实参。也就是说`reference_wrapper`能够被推导为引用。如果`std::bind`的参数`arg`拥有类型`std::reference_wrapper<T>`,在调用的时候会对`arg.get()`得到`T&`,即得到参数的引用。  

总结:`std::ref`生成的类型是`reference_wrapper<T>`而不是`T&`,只是`std::bind`在调用的时候会把`reference_wrapper<T>`转换成`T&`, `std::thread`同理。

`std::bind`的函数体如果需要一个引用的形参，如果`std::bind`默认传的是拷贝，那么函数的实参就是这个拷贝的引用，如果`std::bind`传的是移动，那么函数体的实参就是右值，和他需要的引用不匹配就会编译报错。

`std::ref`和`std::cref`的区别一个是传引用的时候需要用到，一个是传常量引用的时候用到。

注:函数式编程也是对参数直接拷贝，而不是传引用
std::ref和std::cref的实现就是直接裹了一层reference_wrapper
```cc
template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
reference_wrapper<_Tp>
ref(_Tp& __t) _NOEXCEPT
{
    return reference_wrapper<_Tp>(__t);
}
template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
reference_wrapper<const _Tp>
cref(const _Tp& __t) _NOEXCEPT
{
    return reference_wrapper<const _Tp>(__t);
}
```
## std::reference_wrapper
reference_wrapper将引用包装成一个对象，即引用的包装器。可以包裹一个指向对象或者指向函数指针的引用，既可以通过拷贝构造，也可以通过赋值构造。它常用作将引用存储入无法正常保有引用的标准容器的机制(比如上面的thread和bind就不能直接传递引用)。
```cc
template <class _Tp>
class _LIBCPP_TEMPLATE_VIS reference_wrapper
    : public __weak_result_type<_Tp>
{
public:
    // types
    typedef _Tp type;
private:
    type* __f_;

public:
    // construct/copy/destroy
    _LIBCPP_INLINE_VISIBILITY reference_wrapper(type& __f) _NOEXCEPT
        : __f_(_VSTD::addressof(__f)) {}

    // access
    _LIBCPP_INLINE_VISIBILITY operator type&    () const _NOEXCEPT {return *__f_;}
    _LIBCPP_INLINE_VISIBILITY          type& get() const _NOEXCEPT {return *__f_;}

    // invoke
    template <class... _ArgTypes>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_of<type&, _ArgTypes...>::type
    operator() (_ArgTypes&&... __args) const {
        return __invoke(get(), _VSTD::forward<_ArgTypes>(__args)...);
    }
}
```
注:
- 如果reference_wrapper存储的引用是可调用的，则可以用相同的参数调用reference_wrapper对象，将reference_wrapper对象当作可调用对象来调用，当然原理是通过重载的()运算符调用invoke(请参考std::invoke中bullets 7这种调用情况)。
- 可以通过get()方法手动获取reference_wrapper所存储的引用。
## unordered_map和map的区别
map内部的实现是用的红黑树，可以在O(logn)时间内完成查找,插入和删除，在单次时间敏感的场景下建议使用map做为容器。  
unordered_map的内部实现是哈希表，这就决定了，map是一个有序的结构，unordered_map是无序的结构。
在需要有序性或者对单次查询有时间要求的应用场景下，应该使用map，其余情况应使用unordered_map。因为哈希表虽然查询快，但是最坏情况下的查询很慢，因此如果是对单次查询时间有要求，就用map，一定会在O(logn)下查询完成。


...
## 可调用函数对象
* 函数
* 函数指针
* 仿函数(实现了()重载的类)
* lambda表达式
* std::function
* bind创建的对象
## 不可拷贝的对象
C++标准库中很多资源占用类型,比如IO对象std::ifstream，std::unique_ptr，std::thread, std::future都只可以移动，不能拷贝
## std::for_each
先看看llvm中std::for_each源代码的实现
```cc
template<class _InputIterator, class _Function>
inline _LIBCPP_INLINEVISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX17
_Function for_each(_InputIterator __first, _InputIterator __last, _Function __f)
{
    for (; __first != __last; ++__first)
        __f(*__first);
    return __f;
}
```
从代码可以看出来for_each是对迭代器区间[_first,_last)内元素逐个进行__f操作，并且操作完成后返回f,这里的__f是可调用对象，使用例子
```cc
#include <vector>
#include <algorithm>
#include <iostream>
 
struct Sum
{
    void operator()(int n) { sum += n; }
    int sum{0};
};
 
int main()
{
    std::vector<int> nums{3, 4, 2, 8, 15, 267};
 
    auto print = [](const int& n) { std::cout << " " << n; };
 
    std::cout << "before:";
    std::for_each(nums.cbegin(), nums.cend(), print);
    std::cout << '\n';
 
    std::for_each(nums.begin(), nums.end(), [](int &n){ n++; });
 
    // calls Sum::operator() for each number
    Sum s = std::for_each(nums.begin(), nums.end(), Sum());
 
    std::cout << "after: ";
    std::for_each(nums.cbegin(), nums.cend(), print);
    std::cout << '\n';
    std::cout << "sum: " << s.sum << '\n';
}
```
## std::mem_fn
llvm中的实现[点击查看源代码](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/functional#L1402)
std::mem_fn生成指向成员指针的包装对象，它可以存储，复制及调用执行成员指针。到对象的引用和指针(含智能指针)可在调用std::mem_fn时使用
这里结合std::for_each举例(实际上std::mem_fn很多时候是配合std::for_each使用的)
```cc
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <functional>

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([i]{
            for(int j = 0; j < 3; j++)
              printf("%d\n", i);
        });
    }
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    return 0;
}
```
这里对每个线程都执行join操作，这里的jion被std::mem_fn包装成一个对象，实际上就是用这个函数指针构造一个__mem_fn([llvm里面的实现](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/functional#L1279))类对象并返回。
## std::call_once && std::once_flag
有的地方需要代码只执行一次，比如说单例初始化， call_once能保证函数在任何情况下只调用一次，call_once需要配合once_flag使用
```cc
Task* Task::getInstance()
{
    static std::once_flag flag;
    std::call_once(flag, []{
        //这里的代码在多线程里也只会被调用一次
        task = new Task();
    });
    return task;
}
```

## std::invoke
模板函数invoke对于模板元编程非常有用，该模板函数为调用所有C++可调用类型提供统一的语义。[查看介绍](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4169.html) 
```cc
template <class _Fn, class ..._Args>
invoke_result_t<_Fn, _Args...>
invoke(_Fn&& __f, _Args&&... __args)
    noexcept(is_nothrow_invocable_v<_Fn, _Args...>)
{
    return _VSTD::__invoke(_VSTD::forward<_Fn>(__f), _VSTD::forward<_Args>(__args)...);
}
```

__invoke的实现比较长[点击查看libcxx具体实现](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/type_traits#L3437)  
libcxx里面将函数调用分成7种bullets

```
TESTING INVOKE(f, t1, t2, ..., tN)
Bullets 1 -- (t1.*f)(t2, ..., tN)
Bullets 2 -- (t1.get().*f)(t2, ..., tN) //t1 is a reference_wrapper
Bullets 3 -- ((*t1).*f)(t2, ..., tN)
Bullets 4 -- t1.*f
Bullets 5 -- t1.get().*f //t1 is a reference_wrapper
Bullets 6 -- (*t1).*f
Bullets 7 -- f(t1, ..., tN)
/*
* Bullets 1, 2, 3都是处理f是指向成员函数的指针的情况；(成员函数指针)
* Bullets 1 仅仅处理t1是T类型或者是T的派生类型的情况，
* Bullets 2 处理t1是T类型的reference_wrapper的情况，
* Bullets 3 处理其他的情况。
* 
* Bullets 4, 5, 6都是处理f是指向成员对象的指针的情况；(f可能是仿函数，std::function,std::bind,lambda产生的对象)
* Bullets 4 仅仅处理t1是T类型或者是T的派生类型的情况，
* Bullets 5 处理t1是T类型的reference_wrapper的情况,
* Bullets 6 处理其他的情况。
*
* Bullets 7 处理第一个参数不是成员函数的情况
*/
```

从上面可以看出来libcxx将调用分成了7种，成员函数六种(成员函数指针3种，成员函数对象3种)，非成员函数一种。   
使用例子:
```cc
#include <functional>
#include <iostream>
 
struct Foo {
    Foo(int num) : num_(num) {}
    void print_add(int i) const { std::cout << num_+i << '\n'; }
    int num_;
};
 
void print_num(int i)
{
    std::cout << i << '\n';
}
 
struct PrintNum {
    void operator()(int i) const
    {
        std::cout << i << '\n';
    }
};
 
int main()
{
    // 调用自由函数
    std::invoke(print_num, -9);
 
    // 调用 lambda
    std::invoke([]() { print_num(42); });
 
    // 调用成员函数
    const Foo foo(314159);
    std::invoke(&Foo::print_add, foo, 1);
 
    // 调用（访问）数据成员
    std::cout << "num_: " << std::invoke(&Foo::num_, foo) << '\n';
 
    // 调用函数对象
    std::invoke(PrintNum(), 18);
}
```
## new(size_t, void*)

## align & alignas & alignof & alignment_of
### 内存对齐
每个对象类型都有被称为对齐要求的性质，表示这个类型对象在内存中占用的连续相邻地址的字节数(类型std::size_t，总是2的幂，比如char对齐1，int对齐4)  
结构体的对齐
```cc
struct A{ //  32位机器
  char a; //1个字节，对齐4个字节
  int b;  //4个字节，对齐4个字节
  short c;//2个字节，对齐4个字节
}
```
抛开对齐来看，结构体A的大小应该是7个字节，但是为了提高内存的访问效率，比如32位的cpu，每个总线周期都是从偶地址开始读取内存数据，如果不是偶数地址的数据(比如没有对齐的A.b)，则需要两个总线周期才能读取到相要的数据，所以需要将内存中的数据进行对齐放置  
内存对齐规则:  
各成员变量存放的起始地址相对于结构的起始地址的偏移量必须为该变量的类型所占用的字节数的倍数，各成员变量在存放的时候根据在结构中出现的顺序依次申请空间，同时按照上面的对齐方式调整位置，空缺的字节自动填充，同时为了确保结构的大小为结构的字节边界数(即该结构中占用最大的空间的类型的字节数)的倍数，所以在为最后一个成员变量申请空间后，还会根据需要自动填充空缺的字节。
### std::align
```cc
void* align(
  size_t alignment,     //input 欲求的对齐量(2的幂，否则align行为是未定义的)
  size_t size,          //input 要被对齐的存储大小
  void*& ptr,           //input/output 指向至少有space字节的连续存储的指针
  size_t& space         //input/output 要在其中操作的缓冲区的大小
)
```
std::align的作用:将一块给定的内存(起始地址ptr，长度space)，按照我们想要的方式(给定内存中的前size个字节按照alignment大小对齐,要求这size的空间首地址是alignment的倍数,所以可能导致对齐后的首地址会移动位置)进行对齐操作,然后获得符合要求的一块内存地址。如果space太小(小于size或者不够调整对齐空间, 即newptr - ptr + size > space, 请参考源码)，align啥也不干，返回nullptr。否则，ptr变为为对齐后的首地址，space则变为对齐后剩余的调整空间(减去对齐消耗的空间newptr-ptr);  
GCC's `std::align`[implementation](https://github.com/gcc-mirror/gcc/blob/41d6b10e96a1de98e90a7c0378437c3255814b16/libstdc%2B%2B-v3/include/std/memory#L114)  
LLVM's `std::align`[implementation](https://github.com/llvm-mirror/libcxx/blob/6952d1478ddd5a1870079d01f1a0e1eea5b09a1a/src/memory.cpp#L217)

```cc
#include <type_traits> // std::alignment_of()
#include <memory>
//...
char buffer[256]; // for simplicity
size_t alignment = std::alignment_of<int>::value;
void * ptr = buffer;
std::size_t space = sizeof(buffer); // Be sure this results in the true size of your buffer

while (std::align(alignment, sizeof(MyObj), ptr, space)) {
    // You now have storage the size of MyObj, starting at ptr, aligned on
    // int boundary. Use it here if you like, or save off the starting address
    // contained in ptr for later use.
    // ...
    // Last, move starting pointer and decrease available space before
    // the while loop restarts.
    ptr = reinterpret_cast<char*>(ptr) + sizeof(MyObj);
    space -= sizeof(MyObj);
}
// At this point, align() has returned a null pointer, signaling it is not
// possible to allow more aligned storage in this buffer.
```
### alignas & alignof
alignas是用来指定变量或者用户定义类型的对齐方式。  
alignof是获取指定变量或者用户定义类型的对齐方式。
```cc
// alignas_alignof.cpp
// compile with: cl /EHsc alignas_alignof.cpp
#include <iostream>

struct alignas(16) Bar
{
    int i;        // 4 bytes
    int n;        // 4 bytes
    alignas(4) char arr[3];
    short s;      // 2 bytes
};

int main()
{
    std::cout << alignof(Bar) << std::endl; // output: 16
}
```
### alignment_of
alignment_of是对alignof进行了封装，alignment_of类里面包含alignof类型的value，可以通过()获取，即`alignment_of<int>()`

## 数学操作函数
`std::ceil(arg)`表示对arg向上取整  
`std::floor(arg)`表示对arg向下取整  
`std::log2(arg)`表示对arg取2的对数
```cc
std::ceil(5.88)//6 
std::floor(5.88)//5
std::log2(65536)//16
```

## constexpr
constexpr是C++11引入用于简化各种类型的编译器计算，可以用来修饰变量和函数。constexpr函数可以在编译期完成相应的计算，但是C++11版本的constexpr函数通常只能包含一个return语句。这个限制在C++14中被移除了，可以使用常规C++代码中大部分控制结构。  
constexpr指定变量或者函数的值,constexpr用于变量时，变量不可以被修改，这点类似于const，和const不同的是，constexpr可以用于函数和类的构造函数，表示值，返回值都是const的，并且如果可能他们将在编译期间被计算。由于constexpr在编译期就进行了计算，所以编译期可以对constexpr修饰的对象进行优化，这点const也是做不到的。
## nonexcept & nothrow

## weak_ptr


## enum class
## unique_ptr && shared_ptr删除器
```cc
template<class T, class Deleter = std::default_delete<T>> class unique_ptr;
```
通过在编译时绑定删除器，unique_ptr避免了间接调用删除器的运行时开销，通过在运行时绑定删除器，shared_ptr使用户重载删除器更为方便。

## voliate && mutable

## 宏定义中的##和#
\#\#表示连接符
```cc
//WIDE("abc")就会被替换成L"abc"
#define WIDE(str) L##str
```
\#表示串化
```cc
//chSTR2(1 + 1 == 2)会被宏替换为"1 + 1 == 2"而不是计算这个表达式的结果
#define chSTR2(x) #x
```

## string length中的坑
```cc
std::string s = "ABCD";
s[2] = '\0';
std::cout << s << std::endl;                //ABD
std::cout << s.length() << std::endl;       //4
std::cout << strlen(s.c_str()) << std::endl;//2
std::cout << s.c_str()<< std::endl;         //AB
assert(s.size() == strlen(s.c_str()));
```
如果在定义string的时候把string的长度先确定了下来，后续代码中将string中间的字符替换成\0，这个时候中间的\0并不会被识别为字符串结束。因为这个字符串的长度已经有了，并不会影响到他的长度。而且C++没有规定string以\0结尾(得看具体编译器的实现了，大部分编译器都是以\0结束字符串)。但是如果我们调用c_str()，就会以\0结束字符串，因为c_str是获取C风格的字符串。如果需要将string转成C风格的字符串，建议使用c_str,这个一定是会以\0截断string的。

## ptrdiff_t
ptrdiff_t是C/C++标准中定义的与机器相关的数据类型，可以表示两个指针相减结果的有符号整数类型，与size_t不同的是，它可以表示负数，可以用于指针算术及数组下标。

## std::visit

## std::get

## dynamic_pointer_cast

## std::holds_alternative

## std::partition_point