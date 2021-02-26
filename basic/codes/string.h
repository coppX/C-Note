#ifndef __STRING__
#define __STRING__

#include <cstring>
#include <ostream>

class String {
 public:
  // constructor, or ctor
  String (const char* cstr = 0);
  // copy constructor
  String (const String& str);
  // copy assignment operator, or copy op =
  String&
  operator = (const String& str);
  // destructor, or dtor
  ~String();

  char*
  get_c_str () const {return m_data;}

 private:
  char* m_data;
};

// can not redefinition dafault argument here
// default argument should only be defined when
//   functions declared
inline
String::String (const char* cstr) {
  if (cstr) {
    m_data = new char [strlen(cstr)+1];
    strcpy(m_data, cstr);
  } else {
    m_data = new char[1];
    *m_data = '\0';
  }
}

inline
String::~String () {
  delete [] m_data;
}

inline
String::String (const String& str) {
  m_data = new char [strlen(str.m_data)+1];
  strcpy(m_data, str.m_data);
}

inline String&
String::operator = (const String& str) {
  // CHECK SELF ASSIGNMENT
  // or may cause memory leak
  if (this == &str) return *this;

  delete [] m_data;
  m_data = new char [strlen(str.m_data)+1];
  strcpy(m_data, str.m_data);
  return *this;
}

std::ostream&
operator << (std::ostream& os, const String& str) {
  os << str.get_c_str();
  return os;
}

#endif

// Notice:
// 1, always consider returning by ref
// 2, always consider write const functions
// 3, always remember to release memory,
//   use paired new/delete or new[]/delete[] operators
// 4, always consider writing inline functions
// 5, CHECK SELF ASSIGNMENT in copy assignment operator

// Attention!
// 1, function definitions in header files may lead
//   to ODR violations (line 63)

// a simple class with pointers
//   code & note by undr22 in Feb 25th, 2021
