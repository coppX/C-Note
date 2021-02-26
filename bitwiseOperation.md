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

## _builtin_popcount