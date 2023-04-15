#!/bin/bash

assert(){
    expected="$1"
    input="$2"
    ./rvcc "$input" > tmp.s || exit

     # 编译rvcc产生的汇编文件
    
    #使用静态链接编译tmp.s
    $RISCV/bin/riscv64-unknown-linux-gnu-gcc -static -o tmp tmp.s

     # 运行生成出来目标文件
    # ./tmp
    $RISCV/bin/qemu-riscv64 -L $RISCV/sysroot ./tmp

    # 获取程序返回值，存入 实际值
    actual="$?"


    # 判断实际值，是否为预期值
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

# assert 期待值 输入值
# [1] 返回指定数值
# assert 0 '0;'
# assert 42 '42;'

# # [2] 支持+ -运算符
# assert 34 '12-34+56;'

# # [3] 支持空格
# assert 41 ' 12 + 34 - 5 ;'

# # [5] 支持* / ()运算符
# assert 47 '5+6*7;'
# assert 15 '5*(9-6);'
# assert 17 '1-8/(2*2)+3*6;'

# # [6] 支持一元运算的+ -
# assert 10 '-10+20;'
# assert 10 '- -10;'
# assert 10 '- - +10;'
# assert 48 '------12*+++++----++++++++++4;'

# # [7] 支持条件运算符
# assert 0 '0==1;'
# assert 1 '42==42;'
# assert 1 '0!=1;'
# assert 0 '42!=42;'
# assert 1 '0<1;'
# assert 0 '1<1;'
# assert 0 '2<1;'
# assert 1 '0<=1;'
# assert 1 '1<=1;'
# assert 0 '2<=1;'
# assert 1 '1>0;'
# assert 0 '1>1;'
# assert 0 '1>2;'
# assert 1 '1>=0;'
# assert 1 '1>=1;'
# assert 0 '1>=2;'
# assert 1 '5==2+3;'
# assert 0 '6==4+3;'
# assert 1 '0*9+5*2==4+4*(6/3)-2;'


# [10] 支持单字母变量
# assert 3 'a=3; a;'
# assert 8 'a=3; z=5; a+z;'
# assert 6 'a=b=3; a+b;'
# assert 5 'a=3;b=4;a=1;a+b;'
# [9] 支持;分割语句
# assert 3 '{ 1; 2;return 3; }'
# assert 12 '{ 12+23;12+99/3;return 78-66; }'

# # [10] 支持单字母变量
# assert 3 '{ a=3;return a; }'
# assert 8 '{ a=3; z=5;return a+z; }'
# assert 6 '{ a=b=3;return a+b; }'
# assert 5 '{ a=3;b=4;a=1;return a+b; }'

# [12] 支持return
# assert 1 '{ return 1; 2; 3; }'
# assert 2 '{ 1; return 2; 3; }'
# assert 3 '{ 1; 2; return 3; }'

# # [14] 支持空语句
# assert 5 '{ ;;; return 5; }'
# [15] 支持if语句
assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

# [16] 支持for语句
assert 55 '{ i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 '{ for (;;) {return 3;} return 5; }'

# [17] 支持while语句
assert 10 '{ i=0; while(i<10) { i=i+1; } return i; }'

echo OK
