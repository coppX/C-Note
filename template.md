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
TODO  
## std::decay && std::decay_t
## std::common_type && std::common_type_t
## std::is_same
## std::enable_if
## 折叠表达式
## 推导指引
## 类型萃取
## 泛型lambda(C++14起)
C++14引入泛型lambda，是一种成员模板的简化。
```cc
auto lambda = [](auto x, auto y){return x + y;};
```
编译器会默认为它构造下面一个类:
```cc
class SomeCompilerSpecificName {
    public:
    SomeCompilerSpecificName();//constructor only called by compiler
    template<typename T1, template T2>
    auto operator() (T1 x, T2 y) const {//这里的auto是C++14新增的让编译器推断具体的返回类型
        return x + y;
    }
}
```

## SFINAE(Substitution Failure Is Not An Error,即匹配失败不是错误)

