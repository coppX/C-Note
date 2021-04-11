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