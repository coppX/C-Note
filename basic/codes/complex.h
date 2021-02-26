#ifndef __COMPLEX__
#define __COMPLEX__

#include <ostream>

class complex {
 public:
  // constructor, or ctor
  //   (with default arguments here)
  // 1, function name must be same as class name
  // 2, no return type
  // 3, better using initialization list (only avaliable
  //   in ctors), but not assignments
  // 4, could be overloaded, but could only exists ONE
  //   or zero default ctor (default ctor: ctors do not
  //   need provided arguments)
  // 5, could be private, see ./singleton.cpp
  complex (double r=0, double i=0) : re(r), im(i) {}

  // functions defined in class body will be "inline"
  // keyword "inline" only SUGGESTS compiler to make the
  //   function inline, inline or not depends on compiler

  // better passing by ref(erence) (to const)
  // return by value or ref depends on circumstances
  // passer do not have to know reciver recives by
  //   ref
  // return by ref (but not void) allows assignment
  //   in a row
  complex&
  operator += (const complex&);

  // for the functions do not change data members,
  //   JUST ADD KEYWORD "const"
  // otherwise would not operate const objects
  double
  real () const {return re;}
  double
  imag () const {return im;}

 private:
  double re, im;

  // "friend"s can use private members
  // objects in the same class are "friend"s
  friend complex&
  __doapl (complex*, const complex&);
};

// almost all operations could be designed to global
//   functions or member functions, there is not
//   a last word says which one is better,
//   it depends on circumstances

// global function
inline complex&
__doapl (complex* ths, const complex& r) {
  ths->re += r.re;
  ths->im += r.im;
  return *ths;
}

// member function (with classname::)
inline complex&
complex::operator += (const complex& r) {
  // there is always a hidden argument (this) in
  //   member functions, which is a pointer to
  //   the operated object
  return __doapl(this, r);
}

inline double
real (const complex& x) {return x.real();}
inline double
imag (const complex& x) {return x.imag();}

// operator + can not be a member function,
//   for the situation double+complex
inline complex
operator + (const complex& x, const complex& y) {
  // typename() to create temp object
  return complex (real(x) + real(y), imag(x) + imag(y));
}
inline complex
operator + (const complex& x, double y) {
  return complex (real(x) + y, imag(x));
}
inline complex
operator + (double x, const complex& y) {
  return complex (x+ real(y), imag(y));
}

inline complex
operator + (const complex& x) {
  return x;
}
inline complex
operator - (const complex& x) {
  return complex (-real(x), -imag(x));
}

inline bool
operator == (const complex& x, const complex& y) {
  return real(x)==real(y) && imag(x)==imag(y);
}
inline bool
operator == (const complex& x, double y) {
  return real(x)==real(y) && imag(x)==0;
}
inline bool
operator == (double x, const complex& y) {
  return x==real(y) && imag(y)==0;
}

inline bool
operator != (const complex& x, const complex& y) {
  return real(x)!=real(y) || imag(x)!=imag(y);
}
inline bool
operator != (const complex& x, double y) {
  return real(x)!=y || imag(x)!=0;
}
inline bool
operator != (double x, const complex& y) {
  return x!=real(y) || imag(y)!=0;
}

inline complex
conj (const complex& x) {
  return complex (real(x), -imag(x));
}

// operator << must be global
// see line 77
std::ostream&
operator << (std::ostream& os, const complex& x) {
  return os << '(' << real(x) << ',' << imag(x) << ')';
}

#endif

// NOTE:
// 1, use initialization list for ctors
// 2, consider every function whether it should be
//   const
// 3, consider passing by ref (to const) first
// 4, consider returning by value or ref
// 5, set data to private, (most) functions to public

// ATTENTION!
// 1, function definitions in header files may lead
//   to ODR violations (line 136)

// a simple class without pointers,
//   code & note by undr22 in Feb 24th, 2021
