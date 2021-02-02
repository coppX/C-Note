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

## std::move

## std::forward

## std::move和std::forward的区别

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

## std::mem_fn

## std::call_once

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

## SFINAE(Substitution Failure Is Not An Error,即匹配失败不是错误)

## 模板篇
为了生成一个实例化版本，编译器通常需要掌握函数模板或类模板成员函数的定义，函数模板和类模板成员函数的定义通常放在头文件中，模板头文件既包括声明也包括定义。
## std::enable_if


## 可变参数列表

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

## extern "C"

## weak_ptr

## unique_ptr && shared_ptr删除器
```cc
template<class T, class Deleter = std::default_delete<T>> class unique_ptr;
```
通过在编译时绑定删除器，unique_ptr避免了间接调用删除器的运行时开销，通过在运行时绑定删除器，shared_ptr使用户重载删除器更为方便。


## remove_reference

## 引用折叠和右值引用参数
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
