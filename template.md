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
## std::decay类型退化
在向模板传递模板参数的过程中，如果是按值传递参数，那么参数类型就会退化(decay)。也就是说，裸数组(raw array)和函数会退化成相应的指针， cv限制(const和volatile)会被删除。
```cc
template<typename T>
void printV(T arg){
    ...
}
std::string const c = "hi";
printV(c);//T为std::string，顶层const删除
printV("hi");//T为char const*，字符数组char const[3]退化成指针char const*
int arr[4];
printV(arr);//T为int *, int [4]退化成int*
```
虽然按引用传递的方式不会造成退化(decay)，但是并不是在所有情况下都能使用按引用传递，即使在能使用的地方，有时候被推断出来的模板参数类型也会带来不少问题。  
这两种情况各有其优缺点，将数组退化成指针，就不能区分它是指向对象的指针还是一个被传递进来的数组。另一方面，如果被传进来的是字符串常量，那么类型不退化的话就会带来问题，因为不同长度的字符串的类型是不同的。比如
```
template<typename T>
void foo(T const& arg1, T const& arg2){
    ...
}
foo("hi", "guy")；
```
因为字符串常量"hi"的类型是char const [3], 而"guy"的类型是char const [4]，但是这里的函数模板参数要求是类型相同，所以会有编译问题。  
退化在很多情况下是有帮助的，尤其是在需要验证两个对象是否有相同的类型或者可以转换成相同的类型(std::is_same)的时候。  
使用std::decay用来得到退化的类型，libcxx中的decay[实现点击查看](https://github.com/llvm-mirror/libcxx/blob/78d6a7767ed57b50122a161b91f59f19c9bd0d19/include/type_traits#L1351)
```cc
template<class T>
using decay_t = typename dacay<T>::type;// C++ 14新增
```
可以使用decay_t<T>来简化写法
## std::common_type && std::common_type_t
## std::is_same
```cc
template<typename T, typename U>
struct is_same;
//下面是C++17新增
template<typename T, typename U>
inline constexpr bool is_same_v = is_same<T, U>::value;
```
std::is_same用于比较两个类型T和U是不是同一类型，如果是，则内部value为true,这里的T和U会严格比较，包括cv限制。如果用std::decay可以退化掉T和U的类型，然后进行比较，可以判断这两个类型是否是同一类型或者是否能相互转化。
```cc
std::is_same<int, int&>::value; //false
std::is_same_v<int, int&>; //false,C++17用法
std::is_same_v<int, std::decay<int&>>; //true
std::is_same_v<int, std::decay<const int&>>; //true
```
## 折叠表达式
## 推导指引
## 类型萃取(type trait)
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

## 模板的模板参数(Template Template Parameter)
用模板的模板参数，能做到只指定容器类型而不需要指定元素类型
```cc
Stack<int, std::vector<int>> s
//通过模板的模板参数可以写为
Stack<int, std::vector> s;
```
为此必须把第二个模板参数指定为模板的模板参数
```cc
template<typanem T,
    template<typename Elem> class Cont = std::deque>
class Stack {
private:
    Cont<T> elems;
public:
    void push(T const&);
    void pop();
    T const& top() const;
    bool empty() const {
        return elems.empty();
    }
};
```
第二个模板参数是一个类模板，而且它会被第一个模板参数实例化:
```cc
Cont<T> elems;//模板类被第一个参数实例化
```
因为Cont没有用到模板参数Elem，所以可以省略Elem
```cc
template<typename T,
    template<typename> class Cont = std::deque>
```
对于模板的模板参数Cont, C++11之前只能用class关键字修饰, C++11之后可以用别名模板的名称来替代，C++17中可以用typename修饰
```cc
//since C++17
template<typename T,
    template<typename> typename Cont = std::deque>
class Stack {
private:
    Cont<T> v;
}
```
由于默认的std::deque和Cont的参数不匹配，所以可以将代码定义成这样:
```cc
template<typename T, template<typename Elem,
    typename Alloc = std::allocator<Elem>> class Cont = std::deque>
class Stack {
    ...
};
```
这里的Alloc没有用到也可以省略
```cc
template<typename T, template<typename Elem,
    typename = std::allocator<Elem>> class Cont = std::deque>
class Stack {
private:
    Cont<T> elems;
public:
    void push(T const&);
    void pop();
    T const& top() const;
    bool empty const() {
      return elems.empty();
    }
};

template<typename T, typelate<typename, typename>class Cont>
void Stack<T, Cont>::push(T const& elem){
    elems.push_back(elem);
}
...
```
## std::enable_if
使用std::enable_if，可以在某些条件下禁用函数模板。std::enable_if<>是一种类型萃取(type trait)，他会根据一个作为其(第一个)模板参数的编译期表达式决定其行为:
- 如果这个表达式结果为true，它的type成员会返回一个类型:  
1. 如果没有第二个模板参数，返回类型是void。
2. 否则，返回类型是其第二个参数的类型。
- 如果表达式结果是false，则其成员类型是未定义的。更加模板的一个叫做SFINAE的规则，这会导致包含std::enalbe_if<>表达式的函数模板被忽略掉。
```cc
template<typname T>
typename std::enable_if<(sizeof(T) > 4)>::type
foo(){
    ...
}
```
如果sizeof(T)>4成立，函数模板会被展开成
```cc
template<typename T>
void foo(){
    ...
}
```
否则，这个函数模板会被忽略掉。如果我们给std::enable_if<>传递第二个参数:
```cc
template<typename T>
typename std::enable_if<(sizeof(T) > 4), T>::type
foo() {
    return T();
}
```
当sizeof(T) > 4满足时，std::enable_if会被扩展成第二个模板参数。
```cc
template<typename T>
T foo() {
    return T();
}
```
但是由于将enable_if表达式放在声明的中间不是一个明智的做法，因此使用std::enable_if<>的更常见的方法是使用一个额外的、有默认值的模板参数:
```cc
template<typename T, typename = std::enable_if_t<(sizeof(T) > 4)>>
void foo() {

}
```
如果sizeof(T) > 4，它会被展开成:
```cc
template<typename T, typename = void>
void foo() {

}
```
由于从C++14开始所有模板萃取都返回一个类型，因此可以使用一个与之对应的别名模板std::enable_if_t<>，这样就可以省略掉template和::type了。

## if constexpr (C++17)
if constexpr(...)是C++17引入的可以在编译期基于某些条件禁用或启用相应代码的编译期语句。
```cc
template<typename T, typename... Types>
void print(T const& firstArg, Types const&... args) {
    std::cout<< firstArg << '\n';
    if constexpr(sizeof...(args) > 0) {
        print(args...);
    }
}
```
如果args...就是一个空的参数包，此时sizeof...(args)等于0,if语句里面的代码就会在编译期实例化print模板函数的时候丢掉。当然，if constexpr并不仅限于模板函数，可以用于任意类型的函数。
## SFINAE(Substitution Failure Is Not An Error,即替换失败不是错误)

# Two-phase name lookup(二阶段查找)