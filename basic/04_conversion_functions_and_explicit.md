# conversion functions and explicit

### conversion function

```cpp
class Fraction {
 public:
  Fraction ( int num, int den=1 )
    : m_numerator(num), m_denominator(den) {}

  operator double() const {
    return (double)(m_numerator / m_denominator);
  }

 private:
  int m_numerator;
  int m_denominator;
};

Fraction f(3, 5);
double d = 4 + f; // $1
                  // compiler looks for operator + first
                  //   (Nope.)
		  // then looks for if there is a function to make conversion
		  //   from f to a double value
		  //   (Yes!)
		  // then calls operator double()
```

operator double() is a so called "conversion function", allows compiler to use Fraction objects as a double-typed, no agruments or return type. Usually conversion functions' are const.

Could write many conversion functions if sensible, by the way the type after conversion do not have to be a built-in type, the compiler knows will just be fine.

### non-explicit-one-argument ctor


```cpp
class Fraction {
 public:
  Fraction ( int num, int den=1 )
    : m_numerator(num), m_denominator(den) {}

  Fraction operator + ( const Fraction& f ) {
    return Fraction(...);
  }

 private:
  int m_numerator;
  int m_denominator;
};

Fraction f(3, 5);
Fraction d2 = f + 4; // compiler looks for operator + first
                     //   (Yes! But operator + accept a Fraction object)
                     // then looks for if there is a way to make conversion
		     //   form 4 to a Fraction object
		     //   (Yes! The non-explicit-one-argument-ctor could)
		     // then calls non-explicit-ctor
		     // then calls operator +
```

> non-explicit-one-ARGUMENT, not non-explicit-one-PARAMETER

### what if...

... we both have the conversion function and the non-explicit-one-argument ctor the same time?

It may lead to ambiguous error.

```cpp
class Fraction {
 public:
  // the non-explicit-one-agrument-ctor
  Fraction ( int num, int den=1 )
    : m_numerator(num), m_denominator(den) {}

  // the conversion function
  operator double() const {
    return (double)(m_numerator / m_denominator);
  }

  Fraction operator + ( const Fraction& f ) {
    return Fraction(...);
  }

 private:
  int m_numerator;
  int m_denominator;
};

Fraction f(3, 5);
Fraction d3 = f + 4; // $2 // [Error] ambiguous
```

There will be two ways to interpret the ambiguous line:

```
4 converse to Fraction(4) by calling ctor
d3 = f + Fraction(4) by calling operator +
```

or 

```
f converse to 0.6 by calling operator double()
0.6 + 4 = 4.6
d3 = Fraction(4.6) by calling ctor
```

The compiler does not accept two ways to do one thing, so raises ambiguous error.

> Notice the differnece between line $1 and $2

### explicit-one-argument-ctor

```cpp
class Fraction {
 public:
  explicit Fraction ( int num, int den=1 )
    : m_numerator(num), m_denominator(den) {}

  operator double() const {
    return (double)(m_numerator / m_denominator);
  }

  Fraction operator + ( const Fraction& f ) {
    return Fraction(...);
  }

 private:
  int m_numerator;
  int m_denominator;
};

Fraction f(3, 5);
Fraction d4 = f + 4; // [Error] conversion from 'int' to 'Fraction' requested
```

The keyword "explicit" forbids compiler to make conversion in this situation. Or, explicit ctors cannot be used for implicit conversions and copy-initialization.

### an example of conversion function from STL

```cpp
template<class Alloc>
class vector<bool, Alloc> {
 public:
  typedef __bit_reference reference;

 protected:
  reference operator [] (size_type n) // $3
  {
    return *(begin() + differnece_type(n));
  }
  ...
};

struct __bit_reference {
  unsigned int* p;
  unsigned int mask;
  ...
 
 public:
  operator bool() const { return !(!(*p & mask)); } // $4
  ...
};
```

The operator [] in line $3 should have returned a bool value, but a "reference" instead (see proxy design pattern), so there must be a conversion function to converse "reference" objects to bool values (line $4).
