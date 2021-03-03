# pointer-like and function-like classes

### an imperfect pointer-like class

```cpp
template<class T>
class shared_ptr {
 public:
  T& operator * () const { return *px; }
  T* operator -> () const { return px; }

  shared_ptr(T* p) : px(p) {}

 private:
  T* px;
  long* pn;
...
};

sturct Foo {
...
  void method () { ... }
};

shared_ptr<Foo> sp(new Foo);

Foo f(*sp);

sp->method(); // $1
              // px->method();
```

Why does line $1 work? sp calls operator ->, it returns px, how colud px calls method()? The answer is: operator -> is a special operator, its behaviour could deliver, which is sp to px here.

### iterators

An iterator is also a kind of "smart pointer", which should overload more other operators, like operator == and operator ++.

Here's an imperfect implementation:

```cpp
template <class T>
struct __list_node {
  void* prev;
  void* next;
  T data;
};

template<class T, class Ref, class Ptr>
struct __list_iterator {
  typedef __list_iterator<T, Ref, Ptr> self;
  typedef Ptr pointer;
  typedef Ref reference;
  typedef __list_node<T>* link_type;
  link_type node;

  bool operator == ( const self& x) const { return node == x.node; }
  bool operator != ( const self& x) const { return node != x.node; }

  reference operator * () const { return (*node).data; } // $2
  // reference is an alias of T&
  pointer operator -> () const { return &(operator*()); } // $3
  // pointer is an alias of T*

  self& operator ++ () { node = (link_type)((*node).next); return *this; }
  self operator ++ ( int ) { self tmp = *this; ++*this; return tmp; }
  self& operator -- () { node = (link_type)((*node).prev); return *this; }
  self operator -- ( int ) { self tmp = *this; --*this; return tmp; }
};

list<Foo>::iterator ite;
...
*ite; // get a Foo object
ite->method(); // calls Foo::method()
               // equals (*ite).method(); // see line $2
	       // equals (&(*ite))->method(); // see line $3
```

We could show the relationship like this:

```
node-  <-- --  node-  ++ -->  node-
    |              |              |
--- |  ----------- |  ----------- |  ---
  | |  |         | |  |         | |  |
  | V  V         | V  V         | V  V
   prev    ---->  prev    ---->  prev
           |              |
   next ----      next ----      next --

   data      -->  data           data
             |
	 operator *
```

### an imperfect function-like class

How to call a function? Like this: FunctionName()

Actually () is an operator, called "function call operator". So if a "thing" could accept the operator (), we call it a function, or a "function-like".

```cpp
template <class T>
struct identity {
  const T& operator () ( const T& x ) const { return x; }
};

template <class Pair>
struct select1st {
  const typename Pair::first_type&
  operator () ( const Pair& x ) const { return x.first; }
};

template <class Pair>
struct select2nd {
  const typename Pair::second_type&
  operator () ( const Pair& x ) const { return x.second; }
};
```

If a class overloads operator (), the we could say this class's objects are function objects (or functors), as they could be called like a function.

### further

There are many functors in STL, they are small classes, and inherit from some special classes.

There's no data or functions in those special classes, Their size are 0 (1 in relaity).

```cpp
template <class Arg, class Result>
struct unary_function {
  typedef Arg argument_type;
  typedef Result result_type;
};

template <class Arg1, class Arg2, class Result>
struct binary_function {
  typedef Arg1 first_argument_type;
  typedef Arg2 second_argument_type;
  typedef Result result_type;
};
```

```cpp
template <class T>
struct identity : public unary_function<T, T> {
  const T& operator () ( const T& x ) const { return x; }
};

template <class Pair>
struct select1st : public unary_function<T, T> {
  const typename Pair::first_type&
  operator () ( const Pair& x ) const { return x.first; }
};

template <class Pair>
struct select2nd : public unary_function<T, T> {
  const typename Pair::second_type&
  operator () ( const Pair& x ) const { return x.second; }
};

template <class T>
struct plus : public binary_function<T, T, T> {
  T operator () ( const T& x, const T& y) const { return x + y; }
};

template <class T>
struct minus : public binary_function<T, T, T> {
  T operator () ( const T& x, const T& y) const { return x - y; }
};

template <class T>
struct equal_to : public binary_function<T, T, T> {
  T operator () ( const T& x, const T& y) const { return x == y; }
};

template <class T>
struct less : public binary_function<T, T, T> {
  T operator () ( const T& x, const T& y) const { return x < y; }
};
```
