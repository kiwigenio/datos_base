CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3

TARGET = sgbd

SRCS = main.cpp page.cpp storage_manager.cpp buffer_pool_manager.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean: 
	rm -f $(OBJS) $(TARGET) motor_db.bin