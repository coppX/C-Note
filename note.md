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

## 引用折叠

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

## new(size_t, void*) & std::allocator

## std::enable_if

## alignas & alignof

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