# C编译器参数：使用C11标准，生成debug信息，禁止将未初始化的全局变量放入到common段
CFLAGS =-std=c11 -g -fno-common
CC = gcc

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

# rvcc标签，表示如何构建最终的二进制文件，依赖于所有的.o文件
# $@表示目标文件，此处为rvcc，$^表示依赖文件，此处为$(OBJS)
rvcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LDFLAGS)


$(OBJS): rvcc.h

test: rvcc
	./test.sh

clean:
	rm -f rvcc *.o *.s tmp* a.out

.PHONY: test clean