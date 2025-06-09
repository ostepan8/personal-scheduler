# Compiler and flags
CXX      = g++
CXXFLAGS = -std=c++14 -Wall -Iapi -Iexternal/json -Ischeduler



# Source files
SRCS = main.cpp \
	   controller/Controller.cpp \
       model/Model.cpp \
       model/OneTimeEvent.cpp \
       model/RecurringEvent.cpp \
       model/recurrence/DailyRecurrence.cpp \
       model/recurrence/WeeklyRecurrence.cpp \
       model/recurrence/MonthlyRecurrence.cpp \
       model/recurrence/YearlyRecurrence.cpp \
       view/TextualView.cpp \
       api/ApiServer.cpp \
       database/SQLiteScheduleDatabase.cpp \
       scheduler/EventLoop.cpp

# Object files (replace .cpp with .o)
OBJS = $(SRCS:.cpp=.o)

# Executable names
API_TARGET = api_server

# Source files for the MVC command-line application
MVC_SRCS = $(filter-out main.cpp api/ApiServer.cpp,$(SRCS)) main_mvc.cpp
MVC_OBJS = $(MVC_SRCS:.cpp=.o)

# Main build rule: link all object files into the API executable
$(API_TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -lsqlite3 -pthread -o $(API_TARGET)

# Build rule for the MVC command-line executable
mvc: $(MVC_OBJS)
	$(CXX) $(CXXFLAGS) $(MVC_OBJS) -lsqlite3 -pthread -o mvc

.PHONY: mvc

# Alias for building the API server
api: $(API_TARGET)

.PHONY: api

# Rule to compile any .cpp into .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up intermediate and output files
clean:
	rm -f $(OBJS) main_mvc.o $(API_TARGET) mvc \
	$(RECURRENCE_TEST_OBJS) $(EVENT_TEST_OBJS) \
$(MODEL_TEST_OBJS) $(MODEL_COMPREHENSIVE_TEST_OBJS) \
$(CONTROLLER_TEST_OBJS) $(VIEW_TEST_OBJS) $(API_TEST_OBJS) $(TEST_TARGETS)

# Test setup
RECURRENCE_TEST_SRCS = tests/recurrence/recurrence_tests.cpp \
                       model/recurrence/DailyRecurrence.cpp \
                       model/recurrence/WeeklyRecurrence.cpp \
                       model/recurrence/MonthlyRecurrence.cpp \
                       model/recurrence/YearlyRecurrence.cpp
RECURRENCE_TEST_OBJS = $(RECURRENCE_TEST_SRCS:.cpp=.o)
RECURRENCE_TEST_TARGET = recurrence_tests

EVENT_TEST_SRCS = tests/events/event_tests.cpp \
                  model/OneTimeEvent.cpp \
                  model/RecurringEvent.cpp \
                  model/recurrence/DailyRecurrence.cpp \
                  model/recurrence/WeeklyRecurrence.cpp \
                  model/recurrence/MonthlyRecurrence.cpp \
                  model/recurrence/YearlyRecurrence.cpp
EVENT_TEST_OBJS = $(EVENT_TEST_SRCS:.cpp=.o)
EVENT_TEST_TARGET = event_tests

MODEL_TEST_SRCS = tests/model/model_tests.cpp \
                  model/Model.cpp \
                  model/OneTimeEvent.cpp \
                  model/RecurringEvent.cpp \
                  model/recurrence/DailyRecurrence.cpp \
                  model/recurrence/WeeklyRecurrence.cpp \
                  model/recurrence/MonthlyRecurrence.cpp \
                  model/recurrence/YearlyRecurrence.cpp
MODEL_TEST_OBJS = $(MODEL_TEST_SRCS:.cpp=.o)
MODEL_TEST_TARGET = model_tests

MODEL_COMPREHENSIVE_TEST_SRCS = tests/model/model_comprehensive_tests.cpp \
                                model/Model.cpp \
                                model/OneTimeEvent.cpp \
                                model/RecurringEvent.cpp \
                                model/recurrence/DailyRecurrence.cpp \
                                model/recurrence/WeeklyRecurrence.cpp \
                                model/recurrence/MonthlyRecurrence.cpp \
                                model/recurrence/YearlyRecurrence.cpp
MODEL_COMPREHENSIVE_TEST_OBJS = $(MODEL_COMPREHENSIVE_TEST_SRCS:.cpp=.o)
MODEL_COMPREHENSIVE_TEST_TARGET = model_comprehensive_tests

CONTROLLER_TEST_SRCS = tests/controller/controller_tests.cpp \
                       controller/Controller.cpp \
                       model/Model.cpp \
                       model/OneTimeEvent.cpp \
                       model/RecurringEvent.cpp \
                       model/recurrence/DailyRecurrence.cpp \
                       model/recurrence/WeeklyRecurrence.cpp \
                       model/recurrence/MonthlyRecurrence.cpp \
                       model/recurrence/YearlyRecurrence.cpp \
                       scheduler/EventLoop.cpp
CONTROLLER_TEST_OBJS = $(CONTROLLER_TEST_SRCS:.cpp=.o)
CONTROLLER_TEST_TARGET = controller_tests

# View tests
VIEW_TEST_SRCS = tests/view/view_tests.cpp \
                 view/TextualView.cpp \
                 model/Model.cpp \
                 model/OneTimeEvent.cpp \
                 model/RecurringEvent.cpp \
                 model/recurrence/DailyRecurrence.cpp \
                 model/recurrence/WeeklyRecurrence.cpp \
                 model/recurrence/MonthlyRecurrence.cpp \
                 model/recurrence/YearlyRecurrence.cpp
VIEW_TEST_OBJS = $(VIEW_TEST_SRCS:.cpp=.o)
VIEW_TEST_TARGET = view_tests

# API server tests
API_TEST_SRCS = tests/api/api_tests.cpp \
                api/ApiServer.cpp \
                model/Model.cpp \
                model/OneTimeEvent.cpp \
                model/RecurringEvent.cpp \
                model/recurrence/DailyRecurrence.cpp \
                model/recurrence/WeeklyRecurrence.cpp \
                model/recurrence/MonthlyRecurrence.cpp \
                model/recurrence/YearlyRecurrence.cpp
API_TEST_OBJS = $(API_TEST_SRCS:.cpp=.o)
API_TEST_TARGET = api_tests

DATABASE_TEST_SRCS = tests/database/database_tests.cpp \
                     database/SQLiteScheduleDatabase.cpp \
                     model/Model.cpp \
                     model/OneTimeEvent.cpp \
                     model/RecurringEvent.cpp \
                     model/recurrence/DailyRecurrence.cpp \
                     model/recurrence/WeeklyRecurrence.cpp \
                     model/recurrence/MonthlyRecurrence.cpp \
                     model/recurrence/YearlyRecurrence.cpp
DATABASE_TEST_OBJS = $(DATABASE_TEST_SRCS:.cpp=.o)
DATABASE_TEST_TARGET = database_tests

ACTION_REGISTRY_TEST_SRCS = tests/utils/action_registry_tests.cpp
ACTION_REGISTRY_TEST_OBJS = $(ACTION_REGISTRY_TEST_SRCS:.cpp=.o)
ACTION_REGISTRY_TEST_TARGET = action_registry_tests

BUILTIN_ACTIONS_TEST_SRCS = tests/utils/builtin_actions_tests.cpp
BUILTIN_ACTIONS_TEST_OBJS = $(BUILTIN_ACTIONS_TEST_SRCS:.cpp=.o)
BUILTIN_ACTIONS_TEST_TARGET = builtin_actions_tests

NOTIFICATION_REGISTRY_TEST_SRCS = tests/utils/notification_registry_tests.cpp
NOTIFICATION_REGISTRY_TEST_OBJS = $(NOTIFICATION_REGISTRY_TEST_SRCS:.cpp=.o)
NOTIFICATION_REGISTRY_TEST_TARGET = notification_registry_tests

BUILTIN_NOTIFIERS_TEST_SRCS = tests/utils/builtin_notifiers_tests.cpp
BUILTIN_NOTIFIERS_TEST_OBJS = $(BUILTIN_NOTIFIERS_TEST_SRCS:.cpp=.o)
BUILTIN_NOTIFIERS_TEST_TARGET = builtin_notifiers_tests

# Integration tests combining CLI, API and database
INTEGRATION_TEST_SRCS = tests/integration/integration_tests.cpp \
                       controller/Controller.cpp \
                       view/TextualView.cpp \
                       api/ApiServer.cpp \
                       model/Model.cpp \
                       model/OneTimeEvent.cpp \
                       model/RecurringEvent.cpp \
                       model/recurrence/DailyRecurrence.cpp \
                       model/recurrence/WeeklyRecurrence.cpp \
                       model/recurrence/MonthlyRecurrence.cpp \
                       model/recurrence/YearlyRecurrence.cpp \
                       database/SQLiteScheduleDatabase.cpp \
                       scheduler/EventLoop.cpp
INTEGRATION_TEST_OBJS = $(INTEGRATION_TEST_SRCS:.cpp=.o)
INTEGRATION_TEST_TARGET = integration_tests

TEST_TARGETS = $(RECURRENCE_TEST_TARGET) $(EVENT_TEST_TARGET) $(MODEL_TEST_TARGET) $(MODEL_COMPREHENSIVE_TEST_TARGET) $(CONTROLLER_TEST_TARGET) $(VIEW_TEST_TARGET) $(API_TEST_TARGET) $(DATABASE_TEST_TARGET) $(ACTION_REGISTRY_TEST_TARGET) $(BUILTIN_ACTIONS_TEST_TARGET) $(NOTIFICATION_REGISTRY_TEST_TARGET) $(BUILTIN_NOTIFIERS_TEST_TARGET) $(INTEGRATION_TEST_TARGET)

test: $(TEST_TARGETS)
	./run_all_tests.sh
$(RECURRENCE_TEST_TARGET): $(RECURRENCE_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(RECURRENCE_TEST_OBJS) -o $@

$(EVENT_TEST_TARGET): $(EVENT_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(EVENT_TEST_OBJS) -o $@

$(MODEL_TEST_TARGET): $(MODEL_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(MODEL_TEST_OBJS) -o $@

$(MODEL_COMPREHENSIVE_TEST_TARGET): $(MODEL_COMPREHENSIVE_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(MODEL_COMPREHENSIVE_TEST_OBJS) -o $@

$(CONTROLLER_TEST_TARGET): $(CONTROLLER_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(CONTROLLER_TEST_OBJS) -o $@

$(VIEW_TEST_TARGET): $(VIEW_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(VIEW_TEST_OBJS) -o $@

$(API_TEST_TARGET): $(API_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(API_TEST_OBJS) -pthread -o $@

$(DATABASE_TEST_TARGET): $(DATABASE_TEST_OBJS)
	        $(CXX) $(CXXFLAGS) $(DATABASE_TEST_OBJS) -lsqlite3 -o $@

$(ACTION_REGISTRY_TEST_TARGET): $(ACTION_REGISTRY_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(ACTION_REGISTRY_TEST_OBJS) -o $@

$(BUILTIN_ACTIONS_TEST_TARGET): $(BUILTIN_ACTIONS_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(BUILTIN_ACTIONS_TEST_OBJS) -o $@

$(NOTIFICATION_REGISTRY_TEST_TARGET): $(NOTIFICATION_REGISTRY_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(NOTIFICATION_REGISTRY_TEST_OBJS) -o $@

$(BUILTIN_NOTIFIERS_TEST_TARGET): $(BUILTIN_NOTIFIERS_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(BUILTIN_NOTIFIERS_TEST_OBJS) -o $@
	
$(INTEGRATION_TEST_TARGET): $(INTEGRATION_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INTEGRATION_TEST_OBJS) -lsqlite3 -pthread -o $@

.PHONY: test
