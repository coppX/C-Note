## 分支预测
分支预测是CPU在分支指令执行结束之前猜测哪一路分支将会被运行，以提高处理器的指令流水线的性能。CPU找到一个规律，猜到接下来要执行哪一条指令，然后直接跳过去，这样速度就加快了。
```cc
#include <algorithm>
#include <ctime>
#include <iostream>

int main()
{
    // Generate data
    const unsigned arraySize = 32768;
    int data[arraySize];

    for (unsigned c = 0; c < arraySize; ++c)
        data[c] = std::rand() % 256;

    // !!! With this, the next loop runs faster
    std::sort(data, data + arraySize);

    // Test
    clock_t start = clock();
    long long sum = 0;

    for (unsigned i = 0; i < 100000; ++i)
    {
        // Primary loop
        for (unsigned c = 0; c < arraySize; ++c)
        {
            if (data[c] >= 128)
                sum += data[c];
        }
    }

    double elapsedTime = static_cast<double>(clock() - start) / CLOCKS_PER_SEC;

    std::cout << elapsedTime << std::endl;
    std::cout << "sum = " << sum << std::endl;
}
```
上面的代码如果排序了，就会出现下面的情况，在`if(data[c] >= 128)`上分支预测很容易处理
```
T = branch taken
N = branch not taken
data[] = 0, 1, 2, 3, 4, ... 126, 127, 128, 129, 130, ...
branch = N  N  N  N  N  ...  N    N    T    T    T(easy to predict)
```
如果数据没有排序，分支预测就没有作用，无法预测，因为完全没有规律
```
data[] = 226, 185, 125, 158, 198, 144, 217, 79, 202, 118,  14, 150, 177, 182, 133, ...
branch =   T,   T,   N,   T,   T,   T,   T,  N,   T,   N,   N,   T,   T,   T,   N  ...
```
这里代码有分支预测和没有分支预测的代码时间差别很大，不加`std::sort(data, data + arraySize)`的话，时间大概为11.54s，如果加上去，只耗了1.93s。

## debug和release版本的区别

## zero copy

## ABI和API

## copy on write

## 交叉编译

## 惊群效应

## 调用DLL报错__acrt_first_block == header