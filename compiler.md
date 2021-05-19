# 编译相关篇
## extern "C"
在C++代码里面如果需要调用C语言代码，就需要使用extern “C”告诉编译器，接下来的代码使用C语言的方式进行编译。这是因为C和C++编译后的函数名不一样，C++支持重载，编译后的函数名会带上参数类型，C语言编译后的只有函数名不会带有参数类型。
## #pragma unroll/nounroll

## #pragma pack

## #pragma comment

## NDEBUG
assert的行为依赖于一个名为NDEBUG的预处理变量，如果定义了NDEBUG,assert就啥也不做。
```cc
#ifdef NDEBUG

    #define assert(expression) ((void)0)

#else

    _ACRTIMP void __cdecl _wassert(
        _In_z_ wchar_t const* _Message,
        _In_z_ wchar_t const* _File,
        _In_   unsigned       _Line
        );

    #define assert(expression) (void)(                                                       \
            (!!(expression)) ||                                                              \
            (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
        )

#endif
```
## __attribute__ 

## __declspec(dllimport) && __declspec(dllexport)
__declspec是用来指定存储类信息的特性扩展语法，这是MSVC支持的特性扩展语法，不属于标准。dllimport和dllexport是用来从DLL中导入或者向其中导出函数、数据和对象。

```
__declspec( dllimport ) declarator
__declspec( dllexport ) declarator
```
DLL导出:
导出表是给DLL工程用的，告诉DLL工程，哪些符号是对外公开的，哪些符号是私有的。  
导入表示给使用DLL工程用的，告诉使用者，要使用哪些符号。  
如果不用__declspec(dllexport)，那么也可以用DEF文件来实现。  
如果不用__declspec(dllimport)，那么也可以用LIB文件来实现。  
DEF文件需要手写，LIB文件一般随着DLL一起发布。


## OS check
```cc
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#ifdef _WIN64
// win64
#else
// win32
#endif

//-----------------------------
#elif __APPLE__
#include <TargetConditionals.h>
#if defined(TARGET_OS_OSX)
// macos
#elif TARGET_OS_IPHONE
// iphone
#else
// other APPLE os
#endif

//-----------------------------
#elif __ANDROID__
// android

//-----------------------------
#elif __linux__
// linux

#elif __unix__ // all unices not caught above
// Unix

#elif defined(_POSIX_VERSION)
// POSIX
//-----------------------------
#else
// other os
#endif
```

## compiler check
```cc
#if defined(__clang__) || defined(__GNUC__)

	#define CPP_STANDARD __cplusplus
	
#elif defined(_MSC_VER)

	#define CPP_STANDARD _MSVC_LANG
	
#endif
```

## C++ version check
```cc
//CPP_STANDARD的定义参考上面的compiler check
#if CPP_STANDARD >= 199711L     //C++03以下
	#define HAS_CPP_03 1
#endif
#if CPP_STANDARD >= 201103L     //C++11
	#define HAS_CPP_11 1
#endif
#if CPP_STANDARD >= 201402L     //C++14
	#define HAS_CPP_14 1
#endif
#if CPP_STANDARD >= 201703L     //C++17
	#define HAS_CPP_17 1
#endif
#if CPP_STANDARD >= 202002L     //C++20
    #define HAS_CPP_20 1
#endif
```