# 二进制操作
## 位置1(bit-set)
```cc
number |= 1UL << n;
```
这段代码将`number`的第`n`位置1.如果`number`大小大于`unsigned long`，需要把`1UL`换成`1ULL`
## 位置0(bit-clear)
```cc
number &= ~(1UL << n);
```
将`number`的第`n`位赋为0。

## 位置反(bit-toggle)
```cc
number ^= 1UL << n;
```
将`number`的第`n`位置反
## 位检查(bit-check)
```cc
bit = (number >> n) & 1U;
```
先将`number`右移`n`位，然后和1进行与操作，得到的值赋给变量`bit`。如果第`n`位是1，那么`bit`也会变成1；如果`n`为是0，`bit`也会是0


## n & (n - 1)
判断n是否是2的幂，结果0就表示n是2的幂。举例n = 4即0100，n - 1 = 3即0011，n & (n - 1) = 0。如果n = 5即0101,n - 1=4即0100，n & (n - 1) = 0100 = 4 != 0;
## !!
两次取反是将类型转换成相应的bool
```cc
bit = !!number;
```
先将number取反就是false，然后再取反为true，bit就为true；如果number = 0; 取反后就是true，再次取反就是false;

## _builtin_popcount