## 位置1(bit-set)
```cc
number |= 1UL << n;
```
这段代码将number的第n位置1.如果number大小大于unsigned long，需要把1UL换成1ULL
## 位置0(bit-clear)
```cc
number &= ~(1UL << n);
```
将number的第n位赋为0。

## 位置反(bit-toggle)
```cc
number ^= 1UL << n;
```
将number的第n位置反
## 位检查(bit-check)
bit = (number >> n) & 1U;
先将number右移n位，然后和1进行与操作，得到的值赋给变量bit。如果第n位是1，那么bit也会变成1；如果n为是0，bit也会是0

## _builtin_popcount