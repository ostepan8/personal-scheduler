# Compiler and flags
CXX      = g++
CXXFLAGS = -std=c++11 -Wall

# Source files
SRCS = main.cpp \
	   controller/Controller.cpp \
       model/Model.cpp \
       model/OneTimeEvent.cpp \
       model/RecurringEvent.cpp \
       model/recurrence/DailyRecurrence.cpp \
       model/recurrence/WeeklyRecurrence.cpp \
       view/TextualView.cpp

# Object files (replace .cpp with .o)
OBJS = $(SRCS:.cpp=.o)

# Final executable name
TARGET = scheduler

# Main build rule: link all object files into the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Rule to compile any .cpp into .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up intermediate and output files
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET)

# Test setup
TEST_SRCS = tests/recurrence_tests.cpp \
            model/recurrence/DailyRecurrence.cpp \
            model/recurrence/WeeklyRecurrence.cpp 
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_TARGET = recurrence_tests

test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) -o $(TEST_TARGET)

.PHONY: test
