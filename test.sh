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
assert 0 0
assert 42 42
assert 2 "12-10"
echo OK
