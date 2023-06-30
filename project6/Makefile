.SUFFIXES: .cpp .o

CC=g++

SRCDIR=src/
INC=include/
LIBS=lib/

TEST_PATH=test/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

TEST_SRC:=$(TEST_PATH)test.cpp
TEST_OBJ:=$(TEST_PATH)test.o

TEST2_SRC:=$(TEST_PATH)test2.cpp
TEST2_OBJ:=$(TEST_PATH)test2.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)page.cpp $(SRCDIR)file.cpp $(SRCDIR)file_manager.cpp $(SRCDIR)buffer_manager.cpp $(SRCDIR)bpt_node.cpp $(SRCDIR)bpt.cpp $(SRCDIR)bptapi.cpp $(SRCDIR)trx_manager.cpp $(SRCDIR)lock_manager.cpp $(SRCDIR)log_manager.cpp 
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)
HEADER_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

CXXFLAGS+= -g -fPIC -I $(INC) -std=c++17 -W -Wall -pthread

TARGET=main

all: $(TARGET)

$(TARGET): $(TARGET_OBJ) $(HEADER_FOR_LIB) $(OBJS_FOR_LIB)
	make static_library
	$(CC) $(CXXFLAGS) -o $@ $< -L $(LIBS) -lbpt

testa: $(TEST2_OBJ) $(OBJS_FOR_LIB)
	make static_library
	$(CC) $(CXXFLAGS) -o $@ $< -L $(LIBS) -lbpt

testb: $(TEST_OBJ) $(OBJS_FOR_LIB)
	make static_library
	$(CC) $(CXXFLAGS) -g -o test/$@ $< -L $(LIBS) -lbpt

debug: $(TEST_OBJ) $(OBJS_FOR_LIB)
	make static_library
	$(CC) $(CXXFLAGS) -g -o $@ $< -L $(LIBS) -lbpt

prof: $(TARGET_OBJ) $(HEADER_FOR_LIB) $(OBJS_FOR_LIB)
	make static_library
	$(CC) $(CXXFLAGS) -g -pg -o test/$@ $< -L $(LIBS) -lbpt

%.o: %.cpp
	$(CC) $(CXXFLAGS) -o $@ -c $^

clean:
	rm $(TARGET) $(TEST_OBJ) $(TEST2_OBJ) testa debug $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

library:
	g++ -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)