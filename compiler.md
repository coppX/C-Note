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