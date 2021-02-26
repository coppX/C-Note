#include <iostream>
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

int main () {
  President& onlyPresident = President::GetInstance();
  onlyPresident.SetName("Abraham Lincoln");

  // uncomment lines to see how compiler failures
  //   prohibit duplicates

  // President second; // cannot access ctor
  // President* third = new President(); // cannot access ctor
  // President fourth = onlyPresident; // cannot access copy ctor
  // onlyPresident = President::GetInstance(); // cannot access operator =

  // cout is an _IO_ostream_withassign object, based on ostream
  //   ostream class overloads a large number of operator <<
  cout << "The name of the President is:";
  cout << President::GetInstance().GetName() << endl;

  return 0;
}

// a simple singleton class
//   code & node by undr22 in Feb 26th, 2021
