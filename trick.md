## do{}while(0)
这是个用于宏的语法技巧，举个例子
```cpp
#define SAFA_FREE(p) do{delete(p); p=nullptr;}while(0)
```
这里的do{}while(0)的作用就是为了把代码块里面的语句当做一个整体。如果不用这种技巧，则变成了
```cpp
#define SAFA_FREE(p) delete(p); p = nllptr;
```
看以下代码就能区分这两种写法的区别了:
```cpp
if (p)
    SAFE_FREE(p)
else
    p = nullptr;
```
使用第一种写法展开:
```cpp
if (p)
    do{
        delete(p);
        p = nullptr;
    } while(0)
else
    p = nullptr;
```
使用第二种写法展开:
```cpp
if (p)
    delete(p);
    p = nullptr;
else
    p = nullptr;
```
第一种写法中do while循环先执行一次，判断while(0)时结束循环，代码块只会被执行一次，第二种写法就会出现问题，编译都会失败。  
## RAII(Resource Acquistion IS Initialization资源获取即初始化)
RAII的目标就是让变量控制的资源的生命周期和变量的生命周期完全一致，使用方式就是在变量构造的时候获取资源，在变量析构的时候进行资源的释放。这样就避免了资源使用完后没有释放造成泄露。包括文件句柄，网络套接字，数据库链接等资源，而不仅仅是内存的泄露。
