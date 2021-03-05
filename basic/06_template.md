# template

# member template

There are 3 kinds of template: class template, function template, member template.

Compared to class template, there is no need to declare the object type when calls a function template. Function templates will be compiled many times: itself, and being called.

Member template means template in class templates, for higher elasticity:

```cpp
template <class T1, class T2>
struct pair {
  T1 first;
  T2 second;

  pair() : first(T1()), second(T2()) {}
  pair( const T1& a, const T2 b ) : first(a), second(b) {}

  template <class U1, class U2>
  pair( const pair<U1, U2>& p ) : first(p.first), second(p.second) {}
};

class Base1 {};
class Derived1 : public Base1 {};

class Base2 {};
class Derived2 : public Base2 {};

pair<Derived1, Derived2> p;
pair<Base1, Base2> p2(p);
```

```cpp
template<typename _Tp>
class shared_ptr : public __shared_ptr<_Tp> {
  ...
 
  template<typename _Tp1>
  explicit shared_ptr(_Tp1* __p) : __shared_ptr<_Tp>(__p) {}
 
  ...
};

Base1* ptr = new Derived1; // up-cast

shared_ptr<Base1>sptr(new Derived1); // simulate up-cast
```

### specialization

Specialization means a counterwork of generalization.

```cpp
template <class Key>
struct hash {};

template<>
struct hash<char> {
  size_t operator () ( char x ) const { return x; }
};

template<>
struct hash<int> {
  size_t operator () ( int x ) const { return x; }
};

template<>
struct hash<long> {
  size_t operator () ( long x ) const { return x; }
};
```

The example above means we could declare any type for "Key" to use hash class, but if we declare "Key" as char, int, or long, we will get the special behaviour : an overloaded operator ().

### partial specialization and template template parameter

See ../template.md
