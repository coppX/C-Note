# OOP: Composition, Delegation and Inheritance

### composition

```cpp
template <class T, class Sequence = deque<T> >
class queue {
...
 protected:
  Sequence c;

 public:
  bool empty () const { return c.empty(); }

  size_type size () const { return c.size(); }

  reference front () { return c.front(); }

  reference back () { return c.back(); }

  void push ( const value_type& x ) { c.push_back(x); }

  void pop () { c.pop_front(); }
};
```
Composition means "has-a", like code shows above, queue "has-a" Sequence, or deque\<T\> inside.

Class queue doesn't write member functions itself, but call deque\<T\>'s member functions instead. This exapmle could also be called as a "Adapter design pattern".

Container ◆---> Component

For composition relationships, construction is from inside to outside: Container's ctor calls component's default ctor, then execute itself:

```cpp
Container::Container(...) : Component() { ... };
```

Destruction is from ouside to inside: Container's dtor execute itself, then calls component's dtor:

```cpp
Container::~Container(...) { ... ~Component() };
```

### delegation : composition by reference

```cpp
// file String.hpp
class StringRep;
class String {
 public:
  String ();
  String ( const char* s );
  String ( const String& s );
  String& operator = ( const String& s );
  ~String ();

 private:
  StringRep* rep; // pointer to implementation, or pimpl, or handle / body
};
```

```cpp
// file String.cpp
#include "String.hpp"
namespace {
class StringRep {
  friend class String;
  StringRep ( const char* s);
  ~StringRep ();
  int count; // reference counting
  char* rep;
};
}

String::String () { ... }
...
```

String ◇---> StringRep

String as an interface, delegate StringRep to do everything, for the pointer could point to other implementation class, or we could modify the implementation class, but not exert an influence on the interface.

Or this is called a "compile firewall", for the interface could be compiled only once.

### inheritance

```cpp
struct _List_node_base {
  _List_node_base* _M_next;
  _List_node_base* -M_prev;
};

template<typename _Tp>
struct _List_node : public _List_node_base {
  _Tp _M_data;
};
```

Inheritance, means "is-a", like cat "is-a" animal, or \_List\_node is a \_List\_node\_base.

Base

△

|

Derived

> The dtor of base class must be virtual, or there will be ubdefined behaviors.


For inheritance relationships, construction is from inside to outside: Derived class's ctor calls base class's default ctor, then execute itself:

```cpp
Derived::Derived(...) : Base() { ... };
```

Destruction is from ouside to inside: Derived class's dtor execute itself, then calls base class's dtor:

```cpp
Derived::~Derived(...) { ... ~Base() };
```

### inheritance with virtual functions

> For member functions, derived class actually inherits the right of calling.

Divided by virtual or not, there are 3 kinds of member functions:

non-virtual functions: Derived class cannot override;

virtual functions: Derived class could override, and there's a default implementation in base class;

pure virtual functions: Derived class must override, there's not an implementation in base class.

> override: This word could only be used for describing virtual functions in inheritance, differentiate form overload.

```cpp
class Shape {
 public:
  virtual void draw () const = 0; // pure virtual
  virtual void error ( const std::string& msg); // impure virtual
  int objectID () const; // non-virtual
  ...
};

class Rectangle : public Shape { ... };
class Ellipse: public Shape { ... };
```
Here's a simple Template Method design pattern in MFC:

```cpp
#include <iostream>

using namespace std;
 
// application framework
class CDocument {
 public:
  void OnFileOpen () {
    // each cout output is a function or operate in reality
    cout << "dialog..." << endl;
    cout << "check file status..." << endl;
    cout << "open file..." << endl;
    Serialize();
    cout << "close file..." << endl;
    cout << "update all views..." << endl;
  }

  virtual void Serialize () {};
};

// application
class CMyDoc : public CDocument {
 public:
  virtual void Serialize () {
    // only application itself knows how to read its own file
    cout << "CMyDoc::Serialize()" << endl;
  }
};

int main() {
  CMyDoc myDoc;
  myDoc.OnFileOpen();
}
```

### Delegation & Inheritance

An imperfect Observer design pattern:

```cpp
class Subject {
  int m_vlaue;
  vector<Observer*> m_views;
 public:
  void attach ( Observer* obs ) { m_views.push_back(obs); }
  void set_val ( int vlaue ) {
    m_vlaue = value;
    notify();
  }
  void notify () {
    for (int i=0; i<m_views.size(); ++i) {
      m_views[i]->update(this, m_vlaue);
    }
  }
};

class Observer {
 public:
  virtual void update(Subject* sub, int value) = 0;
};
```

Subject ◇---> Observer

                 △

                 |

                ...

An imperfect Composite design pattern:

```cpp
class Component {
  int value;
 public:
  Component ( int val ) { value = val; }
  // add() must be a blank impure virtual function here,
  //   but not a pure virtual function.
  // For we should allow Primitive class inherits Component class
  //   but don't allow Primitive to do an "add" operate.
  virtual void add ( Component* ) {}
};

class Primitive : public Component {
 public:
  Primitive ( int val ) : Component (val) {}
};

class Composite : public Component {
  vector <Component*> c;
 public:
  Composite ( int val ) : Component(val) {}
  void add ( Component* elem ) { c.push_back(elem); }
};
```

      Component <-----------

          △                |

          |                |

    --------------         |

    |            |         |

Primitive    Composite ◇----
