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
