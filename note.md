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

## unordered_map和map的区别

## push_back和emplace_back的区别

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
## std::call_once

## std::invoke
## new(size_t, void*)

## allocator
如果你留意一下就会发现C++ STL中所有的容器都提供了allocator参数来分配内存，STL推荐使用allocator而不是new/delete，例如:
```cc
template<class T, class Allocator = allocator<T>> class vector;
```
C++里面采用new来动态分配内存。默认情况下分配的内存会默认初始化，也就是意味着内置类型或者组合类型的对象值将是未定义的，而类对象会采用默认构造函数来初始化这块内存。
```cc
//默认初始化
string *ps = new string;//初始化为空的string,string类默认构造为空
int *pi = new int;      //pi指向一个未初始化的int,内置类型未定义
//值初始化，类型名后面加括号
int *pi = new int(1024);//pi指向的对象值为1024
int *pi = new int();    //初始化为0
//列表初始化
vector<int> *pv = new vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
```
这对于我们来讲，分配内存的操作不够灵活，所以allocator这个类就诞生了(默认的std::allocator里面采用的operator new来分配空间的，我们可以自定义allocator采用其他分配方式，比如下方例子中的malloc)。allocator定义在memory头文件中，它提供了一种类型感知(需要提供类型)的内存分配方法，它分配的内存是原始的，未构造的。将内存的分配和内存构造分离开来。
### std::allocator
```cc
allocator<string> alloc;            //可以分配string的allocator对象
//分配内存
auto const p = alloc.allocate(n);   //分配n个未初始化的string
auto q = p;
//构造内存,C++20已移除construct
alloc.construct(q++);               //*q为空字符串
alloc.construct(q++, 10, 'c');      //*q为cccccccccc
alloc.construct(q++, "hi");         //*q为hi
//逆序释放,释放后的空间可以再次使用，C++20已移除destroy
while(q != p)
    alloc.destroy(--q);             //释放我们真正构造的string
alloc.deallocate(p, n);             //归还内存给系统
```

### 自定义allocator例子
自定义的allocator必须实现的接口
- 拷贝构造函数
- operator==
- operator!=
- allocate
- deallocate  

[例子来源点击查看](https://en.cppreference.com/w/cpp/named_req/Allocator)
```cc
#include <cstdlib>
#include <new>
#include <limits>
#include <iostream>
#include <vector>
 
template <class T>
struct Mallocator
{
  typedef T value_type;
 
  Mallocator () = default;
  template <class U> constexpr Mallocator (const Mallocator <U>&) noexcept {}
 
  [[nodiscard]] T* allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_alloc();
 
    if (auto p = static_cast<T*>(std::malloc(n*sizeof(T)))) {
      report(p, n);
      return p;
    }
 
    throw std::bad_alloc();
  }
 
  void deallocate(T* p, std::size_t n) noexcept {
    report(p, n, 0);
    std::free(p);
  }
 
private:
  void report(T* p, std::size_t n, bool alloc = true) const {
    std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T)*n
      << " bytes at " << std::hex << std::showbase
      << reinterpret_cast<void*>(p) << std::dec << '\n';
  }
};
 
template <class T, class U>
bool operator==(const Mallocator <T>&, const Mallocator <U>&) { return true; }
template <class T, class U>
bool operator!=(const Mallocator <T>&, const Mallocator <U>&) { return false; }
 
int main()
{
  std::vector<int, Mallocator<int>> v(8);
  v.push_back(42);
}
```

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

## 分支预测

## 数学操作函数
`std::ceil(arg)`表示对arg向上取整  
`std::floor(arg)`表示对arg向下取整  
`std::log2(arg)`表示对arg取2的对数
```cc
std::ceil(5.88)//6 
std::floor(5.88)//5
std::log2(65536)//16
```

## #pragma unroll/nounroll/pack

## constexpr

## nonexcept & nothrow

## extern "C" && #ifdef __cplusplus

## weak_ptr

## do{}while(0)
这是个用于宏的语法技巧，举个例子
```cc
#define SAFA_FREE(p) do{delete(p); p=nullptr;}while(0)
```
这里的do{}while(0)的作用就是为了把代码块里面的语句当做一个整体。如果不用这种技巧，则变成了
```cc
#define SAFA_FREE(p) delete(p); p = nllptr;
```
看以下代码就能区分这两种写法的区别了:
```cc
if (p)
    SAFE_FREE(p)
else
    p = nullptr;
```
使用第一种写法展开:
```cc
if (p)
    do{
        delete(p);
        p = nullptr;
    } while(0)
else
    p = nullptr;
```
使用第二种写法展开:
```cc
if (p)
    delete(p);
    p = nullptr;
else
    p = nullptr;
```
第一种写法中do while循环先执行一次，判断while(0)时结束循环，代码块只会被执行一次，第二种写法就会出现问题，编译都会失败。
## unique_ptr && shared_ptr删除器
```cc
template<class T, class Deleter = std::default_delete<T>> class unique_ptr;
```
通过在编译时绑定删除器，unique_ptr避免了间接调用删除器的运行时开销，通过在运行时绑定删除器，shared_ptr使用户重载删除器更为方便。

# 模板篇
为了生成一个实例化版本，编译器通常需要掌握函数模板或类模板成员函数的定义，函数模板和类模板成员函数的定义通常放在头文件中，模板头文件既包括声明也包括定义。
## 引用折叠和右值引用参数

### 左值 & 右值
当一个对象被用作右值的时候，用的是对象的值(内容),当对象被用作左值的时候，用的是对象的身份(在内存中的位置)  
返回左值引用的函数，连同赋值、下标、解引用和前置递增/递减运算符，都是返回左值的表达式的例子。我们可以将一个左值引用绑定到这类表达式的结果上。  
返回非引用类型的函数，连同算术、关系、位以及后置的递增/递减运算符，都生成右值。我们不能将一个左值引用绑定到这类表达式上，但是我们可以将一个const的左值引用(这样的话，该引用也就不可以修改引用的对象，毕竟右值不能修改)或者一个右值引用绑定到这类表达式上。  
### 左值持久；右值短暂
左值有持久的状态，而右值要么是字面常亮，要么是在表达式求值过程中创建的临时对象。所以右值引用可以自由的接管所引用的对象的资源(因为右值是没有其他的用户在使用的)。  
虽然不能将一个右值引用直接绑定到一个左值上，但是我们可以显示地将一个左值转换为对应的右值引用类型，我们还可以通过std::move来获得绑定到左值上的右值引用。
```cc
template<typename T> void f3(T&&);
f3(42); //实参是一个int类型的右值;模板参数T是int
f3(i);  //i是一个int类型的左值，T为int&
```
通常情况下，我们不能将一个右值引用绑定到一个左值上，但是C++在正常绑定规则之外定义了两个例外规则，允许这种绑定。这两个例外规则是move这种标准库设施正确工作的基础。
- 当我们将一个左值传递给函数的右值引用参数，且此右值引用指向模板参数时，编译器推断模板类型参数为实参的左值引用类型。因此我们调用f3(i)时，编译器推断T的类型问int&，而非int。看起来f3的函数参数是一个int&的右值引用。通常，我们不能(直接)定义一个引用的引用。但是通过类型别名或者通过模板类型参数间接定义是可以的。
- 如果我们间接创建一个引用的引用，则这些引用形成了“折叠”。在所有情况下(除了一个例外)，引用会折叠成一个普通的左值引用类型。在新标准中，折叠规则扩展到右值引用。只在一种特殊情况下引用会折叠成右值引用:右值引用的右值引用。即，对于一个给定类型X:
```
X& &、X& &&和X&& &都会折叠成类型X&
类型X&& &&折叠成x&&
```
引用折叠只能应用于间接创建的引用的引用，如类型别名或者模板参数  

这两个规则导致了两个重要结果:
- 如果一个函数参数是一个指向模板类型参数的右值引用(如，T&&)，则它可以被绑定到一个左值；且
- 如果实参是一个左值，则推断出来的模板实参类型将是一个左值引用，且函数参数将被实例化为一个(普通)左值引用参数(T&)

如果一个函数参数是指向模板类型参数的右值引用(如,`T&&`)，它对应的实参的const属性和左值/右值属性将得到保持。
## remove_reference
我们用`remove_reference`来获得元素类型。`remove_reference`模板有一个模板类型参数和一个名为type(public，remove_reference是个struct)的类型成员。如果我们用一个引用类型实例化`remove_reference`,则type表示被引用的类型，例如：`remove_reference<int&>::type`就是`int`。
## std::move
我们不能直接将一个右值引用绑定到一个左值上，但是`std::move`可以帮我们做到这件事，标准库是这样定义`std::move`的：
```cc
template<typename T>
typename remove_reference<T>::type&& move(T&& t)
{
  return static_cast<typename remove_reference<T>::type&&>(t);
}
```
`std::move`是一个模板函数，它可以接受任何类型的实参，因为他的函数参数T&&是一个指向模板类型参数的右值引用。通过引用折叠，此参数可以与任何类型的实参(左值右值都可以)匹配。  
返回类型是模板参数T类型的右值引用，返回参数前面加上typename关键字是因为作用域运算符::后面跟的可能是类型或者是static数据成员，我们用typename来显示指示这里的`remove_reference<T>::type`是一个类型而不是static数据成员，`remove_reference`函数会去掉T的引用属性(如果T是引用类型会得到T所引用对象)。  
这里的左值往右值转换是由`static_cast`来完成的，虽然不能隐式地将一个左值转换成右值，但是我们可以用`static_cast`来显示的转换。

## std::forward完美转发
与`std::move`不同，`std::forward`必须通过显式模板实参来调用。`std::forward`返回该显示实参类型的右值引用。即，`std::forward<T>`的返回类型是`T&&`。当用于一个指向模板参数类型的右值引用函数参数(`T&&`)时，`std::forward`会保持实参类型的所有细节。
```cc
template<typename Type> intermediary(Type &&arg)
{
  finalFcn(std::forward<Type>(arg));
}
```

例子中我们使用Type作为forward的显示模板实参类型，它是从arg推断出来的。由于arg是一个模板类型参数的右值引用，Type将表示传递给arg的实参的所有类型信息。如果实参是一个右值(如，`int&&`)，则Type是一个普通(非引用)类型`int`,forward<int>将返回`int&&`，如果实参是一个左值(如，`int&`，则通过引用折叠，Type本身是一个左值引用类型(`int&`)。在此情况下，返回类型是一个指向左值引用类型的右值引用。即`int&& &`，经过引用折叠后返回的类型就变成了`T&`。
## 可变参数模板...
一个可变参数模板就是接受可变数目参数的模板函数或者模板类。可变数目的参数被称为参数包。存在两种参数包:
- 模板参数包，表示零个或者多个模板参数
- 函数参数包，表示零个或者多个函数参数
  参数包的表示是用省略号`...`来表示的,模板中就是`class...`或者`typename...`。如:
  ```cc
  template<typename T, typename... Args>
  void foo(const T &t, const Args& ... rest);
  ```
这里的Args是模板参数包，rest就是函数参数包。可以表示数目不确定类型不确定的参数，编译器会通过调用这个函数的实参来推断模板参数类型和数量。
如果我们需要在代码里面拿到这个参数包里面具体有多少个参数(每次调用参数数量都可能不一样)，可以采用`sizeof...`操作符
```cc
template<typename... Args>
void g(Args ... args) {
  cout << sizeof...(Args) <<endl;   //类型参数的数目
  cout << sizeof...(args) <<endl;   //函数参数的数目
}
```


## std::enable_if

## SFINAE(Substitution Failure Is Not An Error,即匹配失败不是错误)

