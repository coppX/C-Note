# 编译相关篇
## extern "C" && #ifdef __cplusplus

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