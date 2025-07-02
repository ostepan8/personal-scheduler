# Compiler and flags
CXX       = g++
CXXFLAGS  = -std=c++14 -Wall -Iapi -Iexternal/json -Ischeduler

# Libraries to link
LIBS      = -lsqlite3 -pthread -lcurl

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
       api/routes/EventRoutes.cpp \
       api/routes/AvailabilityRoutes.cpp \
       api/routes/StatsRoutes.cpp \
       api/routes/RecurringRoutes.cpp \
       database/SQLiteScheduleDatabase.cpp \
       scheduler/EventLoop.cpp \
       calendar/GoogleCalendarApi.cpp \
       utils/EnvLoader.cpp \
       security/Auth.cpp \
       security/RateLimiter.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable names
API_TARGET = api_server

# MVC command-line application sources (exclude API/server routes)
MVC_SRCS = $(filter-out \
             main.cpp \
             api/ApiServer.cpp \
             api/routes/EventRoutes.cpp \
             api/routes/AvailabilityRoutes.cpp \
             api/routes/StatsRoutes.cpp \
             api/routes/RecurringRoutes.cpp, \
           $(SRCS)) \
           main_mvc.cpp
MVC_OBJS = $(MVC_SRCS:.cpp=.o)

# Default build: API server
$(API_TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $(API_TARGET)

# Build MVC CLI tool
mvc: $(MVC_OBJS)
	$(CXX) $(CXXFLAGS) $(MVC_OBJS) $(LIBS) -o mvc

.PHONY: mvc api

# Alias for building the API server
api: $(API_TARGET)

# Compile any .cpp into .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up builds and tests
clean:
	rm -f $(OBJS) main_mvc.o $(API_TARGET) mvc \
	      $(RECURRENCE_TEST_OBJS) $(EVENT_TEST_OBJS) \
	      $(MODEL_TEST_OBJS) $(MODEL_COMPREHENSIVE_TEST_OBJS) \
	      $(CONTROLLER_TEST_OBJS) $(VIEW_TEST_OBJS) $(API_TEST_OBJS) \
	      $(DATABASE_TEST_OBJS) $(ACTION_REGISTRY_TEST_OBJS) \
	      $(BUILTIN_ACTIONS_TEST_OBJS) $(NOTIFICATION_REGISTRY_TEST_OBJS) \
	      $(BUILTIN_NOTIFIERS_TEST_OBJS) $(GCAL_TEST_OBJS) $(INTEGRATION_TEST_OBJS) $(TEST_TARGETS)

# ------------------------
# Test setup
# ------------------------

# Recurrence tests
RECURRENCE_TEST_SRCS = tests/recurrence/recurrence_tests.cpp \
                       model/recurrence/DailyRecurrence.cpp \
                       model/recurrence/WeeklyRecurrence.cpp \
                       model/recurrence/MonthlyRecurrence.cpp \
                       model/recurrence/YearlyRecurrence.cpp
RECURRENCE_TEST_OBJS = $(RECURRENCE_TEST_SRCS:.cpp=.o)
RECURRENCE_TEST_TARGET = recurrence_tests

# Event tests
EVENT_TEST_SRCS = tests/events/event_tests.cpp \
                  model/OneTimeEvent.cpp \
                  model/RecurringEvent.cpp \
                  model/recurrence/DailyRecurrence.cpp \
                  model/recurrence/WeeklyRecurrence.cpp \
                  model/recurrence/MonthlyRecurrence.cpp \
                  model/recurrence/YearlyRecurrence.cpp \
                  calendar/GoogleCalendarApi.cpp
EVENT_TEST_OBJS = $(EVENT_TEST_SRCS:.cpp=.o)
EVENT_TEST_TARGET = event_tests

# Model tests
MODEL_TEST_SRCS = tests/model/model_tests.cpp \
                  model/Model.cpp \
                  model/OneTimeEvent.cpp \
                  model/RecurringEvent.cpp \
                  model/recurrence/DailyRecurrence.cpp \
                  model/recurrence/WeeklyRecurrence.cpp \
                  model/recurrence/MonthlyRecurrence.cpp \
                  model/recurrence/YearlyRecurrence.cpp \
                  calendar/GoogleCalendarApi.cpp
MODEL_TEST_OBJS = $(MODEL_TEST_SRCS:.cpp=.o)
MODEL_TEST_TARGET = model_tests

# Model comprehensive tests
MODEL_COMPREHENSIVE_TEST_SRCS = tests/model/model_comprehensive_tests.cpp \
                                model/Model.cpp \
                                model/OneTimeEvent.cpp \
                                model/RecurringEvent.cpp \
                                model/recurrence/DailyRecurrence.cpp \
                                model/recurrence/WeeklyRecurrence.cpp \
                                model/recurrence/MonthlyRecurrence.cpp \
                                model/recurrence/YearlyRecurrence.cpp \
                                calendar/GoogleCalendarApi.cpp
MODEL_COMPREHENSIVE_TEST_OBJS = $(MODEL_COMPREHENSIVE_TEST_SRCS:.cpp=.o)
MODEL_COMPREHENSIVE_TEST_TARGET = model_comprehensive_tests

# Controller tests
CONTROLLER_TEST_SRCS = tests/controller/controller_tests.cpp \
                       controller/Controller.cpp \
                       model/Model.cpp \
                       model/OneTimeEvent.cpp \
                       model/RecurringEvent.cpp \
                       model/recurrence/DailyRecurrence.cpp \
                       model/recurrence/WeeklyRecurrence.cpp \
                       model/recurrence/MonthlyRecurrence.cpp \
                       model/recurrence/YearlyRecurrence.cpp \
                       scheduler/EventLoop.cpp \
                       calendar/GoogleCalendarApi.cpp
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
                 model/recurrence/YearlyRecurrence.cpp \
                 calendar/GoogleCalendarApi.cpp
VIEW_TEST_OBJS = $(VIEW_TEST_SRCS:.cpp=.o)
VIEW_TEST_TARGET = view_tests

# API tests
API_TEST_SRCS = tests/api/api_tests.cpp \
                api/ApiServer.cpp \
                api/routes/EventRoutes.cpp \
                api/routes/AvailabilityRoutes.cpp \
                api/routes/StatsRoutes.cpp \
                api/routes/RecurringRoutes.cpp \
                utils/EnvLoader.cpp \
                security/Auth.cpp \
                security/RateLimiter.cpp \
                model/Model.cpp \
                model/OneTimeEvent.cpp \
                model/RecurringEvent.cpp \
                model/recurrence/DailyRecurrence.cpp \
                model/recurrence/WeeklyRecurrence.cpp \
                model/recurrence/MonthlyRecurrence.cpp \
                model/recurrence/YearlyRecurrence.cpp \
                calendar/GoogleCalendarApi.cpp
API_TEST_OBJS = $(API_TEST_SRCS:.cpp=.o)
API_TEST_TARGET = api_tests

# Database tests
DATABASE_TEST_SRCS = tests/database/database_tests.cpp \
                     database/SQLiteScheduleDatabase.cpp \
                     model/Model.cpp \
                     model/OneTimeEvent.cpp \
                     model/RecurringEvent.cpp \
                     model/recurrence/DailyRecurrence.cpp \
                     model/recurrence/WeeklyRecurrence.cpp \
                     model/recurrence/MonthlyRecurrence.cpp \
                     model/recurrence/YearlyRecurrence.cpp \
                     calendar/GoogleCalendarApi.cpp
DATABASE_TEST_OBJS = $(DATABASE_TEST_SRCS:.cpp=.o)
DATABASE_TEST_TARGET = database_tests

# Utility tests
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

# Google Calendar tests
GCAL_TEST_SRCS = tests/calendar/google_calendar_api_tests.cpp \
                 calendar/GoogleCalendarApi.cpp \
                 model/OneTimeEvent.cpp \
                 model/RecurringEvent.cpp \
                 model/recurrence/DailyRecurrence.cpp \
                 model/recurrence/WeeklyRecurrence.cpp \
                 model/recurrence/MonthlyRecurrence.cpp \
                 model/recurrence/YearlyRecurrence.cpp
GCAL_TEST_OBJS = $(GCAL_TEST_SRCS:.cpp=.o)
GCAL_TEST_TARGET = google_calendar_api_tests

# Integration tests
INTEGRATION_TEST_SRCS = tests/integration/integration_tests.cpp \
                       controller/Controller.cpp \
                       view/TextualView.cpp \
                       api/ApiServer.cpp \
                       api/routes/EventRoutes.cpp \
                       api/routes/AvailabilityRoutes.cpp \
                       api/routes/StatsRoutes.cpp \
                       api/routes/RecurringRoutes.cpp \
                       utils/EnvLoader.cpp \
                       security/Auth.cpp \
                       security/RateLimiter.cpp \
                       model/Model.cpp \
                       model/OneTimeEvent.cpp \
                       model/RecurringEvent.cpp \
                       model/recurrence/DailyRecurrence.cpp \
                       model/recurrence/WeeklyRecurrence.cpp \
                       model/recurrence/MonthlyRecurrence.cpp \
                       model/recurrence/YearlyRecurrence.cpp \
                       database/SQLiteScheduleDatabase.cpp \
                       scheduler/EventLoop.cpp \
                       calendar/GoogleCalendarApi.cpp
INTEGRATION_TEST_OBJS = $(INTEGRATION_TEST_SRCS:.cpp=.o)
INTEGRATION_TEST_TARGET = integration_tests

# All test targets
TEST_TARGETS = $(RECURRENCE_TEST_TARGET) \
               $(EVENT_TEST_TARGET) \
               $(MODEL_TEST_TARGET) \
               $(MODEL_COMPREHENSIVE_TEST_TARGET) \
               $(CONTROLLER_TEST_TARGET) \
               $(VIEW_TEST_TARGET) \
               $(API_TEST_TARGET) \
               $(DATABASE_TEST_TARGET) \
               $(ACTION_REGISTRY_TEST_TARGET) \
               $(BUILTIN_ACTIONS_TEST_TARGET) \
               $(NOTIFICATION_REGISTRY_TEST_TARGET) \
               $(BUILTIN_NOTIFIERS_TEST_TARGET) \
               $(GCAL_TEST_TARGET) \
               $(INTEGRATION_TEST_TARGET)

# Build and run all tests
test: $(TEST_TARGETS)
	./run_all_tests.sh

# Individual test rules
$(RECURRENCE_TEST_TARGET): $(RECURRENCE_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(RECURRENCE_TEST_OBJS) $(LIBS) -o $@

$(EVENT_TEST_TARGET): $(EVENT_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(EVENT_TEST_OBJS) $(LIBS) -o $@

$(MODEL_TEST_TARGET): $(MODEL_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(MODEL_TEST_OBJS) $(LIBS) -o $@

$(MODEL_COMPREHENSIVE_TEST_TARGET): $(MODEL_COMPREHENSIVE_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(MODEL_COMPREHENSIVE_TEST_OBJS) $(LIBS) -o $@

$(CONTROLLER_TEST_TARGET): $(CONTROLLER_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(CONTROLLER_TEST_OBJS) $(LIBS) -o $@

$(VIEW_TEST_TARGET): $(VIEW_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(VIEW_TEST_OBJS) $(LIBS) -o $@

$(API_TEST_TARGET): $(API_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(API_TEST_OBJS) $(LIBS) -o $@

$(DATABASE_TEST_TARGET): $(DATABASE_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(DATABASE_TEST_OBJS) $(LIBS) -o $@

$(ACTION_REGISTRY_TEST_TARGET): $(ACTION_REGISTRY_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(ACTION_REGISTRY_TEST_OBJS) $(LIBS) -o $@

$(BUILTIN_ACTIONS_TEST_TARGET): $(BUILTIN_ACTIONS_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(BUILTIN_ACTIONS_TEST_OBJS) $(LIBS) -o $@

$(NOTIFICATION_REGISTRY_TEST_TARGET): $(NOTIFICATION_REGISTRY_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(NOTIFICATION_REGISTRY_TEST_OBJS) $(LIBS) -o $@

$(BUILTIN_NOTIFIERS_TEST_TARGET): $(BUILTIN_NOTIFIERS_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(BUILTIN_NOTIFIERS_TEST_OBJS) $(LIBS) -o $@

$(GCAL_TEST_TARGET): $(GCAL_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(GCAL_TEST_OBJS) $(LIBS) -o $@

$(INTEGRATION_TEST_TARGET): $(INTEGRATION_TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INTEGRATION_TEST_OBJS) $(LIBS) -o $@
