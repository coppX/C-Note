# Static members

### "this" pointer

```cpp
class complex {
 public:
  double real () const {
    return re; // $1-1
    // return this -> re; // $1-2
 }

 private:
  double re, im;
}

complex c1, c2;
cout << c1.real(); // $2-1
cout << c2.real();
// cout << complex::real(&c1); // $2-2
// cout << complex::real(&c2);
```

All member functions contain a hidden argument "this", which is a pointer to the object who calls the function.

Line $1-1 and $1-2 just do a same thing: return the object's data member "re".

In line $2-1, c1 uses the dot operator to call real() function, could also be written as line $2-2 does: call real() by passing the object address, which is the "this" pointer.

### static members

There could be 4 kinds of members in a class: data members, static data members, member functions, static member functions.

If we create 3 objects, there will be 3 data members, for 1 (two double-typed data for complex) in each objects. But there will only be ONE copy of (static) member functions, member functions use address of different objects (this pointer) to operate ojects.

For static data members, there will be only ONE copy, too. For the using of, we want to design a class of bank accounts, for example. The interest rate should be same to everyone, so it should be a static data member.

The difference between static member functions and common member functions is, there's no "this" pointer in static ones. For this reason they cannot operate data members, but only can operate static data members.

```cpp
class Account {
 public:
  static double m_rate; // declaration $3
  static void
  set_rate (const double& x) { m_rate = x; }
};

double Account::m_rate = 8.0; // definition // $4

int main() {
  Account::set_rate(5.0); // $5

  Account a;
  a.set_rate(7.0); // $6
}
```

For static data members, you should add a definition line outside the class (line $4), line $3 is just a declaration, for the static data member detached from objects.

> Definition means an object gets memory.

As the code shows, there are two ways to call static member functions: by class name (line $5), or by objects (line $6). Notice that the statement in line $6 does not pass a this pointer to set\_rate().

### A use of static members: the Singleton design pattern

```cpp
#include <string>

using namespace std;

class President {
 private:
  President (); // private default constructor
  President (const President&); // private copy constructor
  const President&
  operator = (const President&); // assignment operator

  string name;

 public:
  static President&
  GetInstance () {
    // static objects are constructed only once
    static President onlyInstance;
    return onlyInstance;
  }

  string
  GetName () { return name; }

  void
  SetName (string InputName) { name = InputName; }
};
```

See the complete code and comments in [singleton.cpp](./codes/singleton.cpp).

[Next->](./03_OOP_basic.md)
