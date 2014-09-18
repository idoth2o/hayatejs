# resolve tool name
ifeq ($(OS),Windows_NT)
# for Windows
else
UNAME = ${shell uname}
ifeq ($(UNAME),Linux)
# for Linux
endif
ifeq ($(UNAME),Darwin)
# for MacOSX

endif
endif

CXX =g++
LDFLAG=-lv8 -g -levent
CPPFLAG=-std=c++0x -Wall

TARGET = hayate
OBJS =  thread.o 
DEPS := $(SRCS:%.c=%.d)

.SUFFIXES: .cc .o

# デフォルト動作

# コマンド
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAG) -o $(TARGET)

.cc.o:
	$(CXX) $(CPPFLAG) -c $<

.PHONY: clean
clean :
	-$(RM) *.o
	-$(RM) $(TARGET)
