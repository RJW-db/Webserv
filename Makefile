NAME			:=	Webserv

#	Get the number of logical processors (threads)
OS				:=	$(shell uname -s)
ifeq ($(OS), Linux)
	N_JOBS		:=	$(shell nproc)
else ifeq ($(OS), Darwin)
	N_JOBS		:=	$(shell sysctl -n hw.logicalcpu)
else
	N_JOBS		:=	1
endif

#	(-j) Specify the number of jobs (commands) to run simultaneously
MULTI_THREADED	:=	-j$(N_JOBS)

#	MAKEFLAGS will automatically apply the specified options (e.g., parallel execution) when 'make' is invoked
MAKEFLAGS		+=	$(MULTI_THREADED)

RM				:=	rm -rf
PRINT_NO_DIR	:=	--no-print-directory

#		CCPFLAGS for testing
COMPILER		:=	c++
CCPFLAGS		:=	-std=c++17
CCPFLAGS		+=	-Wall -Wextra
# CCPFLAGS		+=	-Werror
# CCPFLAGS		+=	-Wunreachable-code -Wpedantic -Wconversion -Wshadow
CCPFLAGS		+=	-MMD -MP
CCPFLAGS		+=	-g
#		Werror cannot go together with fsanitize, because fsanitize won't work correctly.
# CCPFLAGS		+=	-fsanitize=address

#		Directories
BUILD_DIR		:=	.build/
INCD			:=	include/

#		SOURCE FILES
SRC_DIR			:=	src/

MAIN			:=	main.cpp						server.cpp							serverListenFD.cpp		\
					parsing.cpp						ConfigServer.cpp			Aconfig.cpp		FileDescriptor.cpp				\
					HttpRequest.cpp					loggingErrors.cpp					Location.cpp			\
					examples/poll_usage.cpp			examples/getaddrinfo_usage.cpp 		examples/server.cpp
# PARSE			:=	parse/parsing.cpp				parse/parse_utils.cpp

#		Find all .c files in the specified directories
SRCP			:=	$(addprefix $(SRC_DIR), $(MAIN))															
# $(addprefix $(SRC_DIR)parsing/, $(PARSE))

#		Generate object file names
OBJS 			:=	$(SRCP:%.cpp=$(BUILD_DIR)%.o)
#		Generate Dependency files
DEPS			:=	$(OBJS:.o=.d)

#		HEADERS
INCS			:=	Webserv.hpp			Parsing.hpp		ConfigServer.hpp		Location.hpp		Aconfig.hpp
HEADERS			:=	$(addprefix $(INCD), $(INCS))
INCLUDE_WEB		:=	-I $(INCD)

BUILD			:=	$(COMPILER) $(INCLUDE_WEB) $(CCPFLAGS)

#		Remove these created files
DELETE			:=	*.out			**/*.out			.DS_Store												\
					**/.DS_Store	.dSYM/				**/.dSYM/

#		RECIPES
all:	$(NAME)

$(NAME): $(OBJS)
	$(BUILD) $(OBJS) -o $(NAME)
	@printf "$(CREATED)" $@ $(CUR_DIR)

$(BUILD_DIR)%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(BUILD) $(INCLUDE_WEB) -c $< -o $@

clean:
	@$(RM) $(BUILD_DIR) $(DELETE)
	@printf "$(REMOVED)" $(BUILD_DIR) $(CUR_DIR)$(BUILD_DIR)

fclean:	clean
	@$(RM) $(NAME)
	@printf "$(REMOVED)" $(NAME) $(CUR_DIR)

re:
	$(MAKE) $(PRINT_NO_DIR) fclean
	$(MAKE) $(PRINT_NO_DIR) all

test: all
	./$(NAME)

valgrind: all
	valgrind --track-fds=yes ./$(NAME)

print-%:
	$(info $($*))

#		Include the dependency files
-include $(DEPS)

.PHONY: all clean fclean re test valgrind

# ----------------------------------- colors --------------------------------- #
BOLD		= \033[1m
DIM			= \033[2m
ITALIC		= \033[3m
UNDERLINE	= \033[4m
BLACK		= \033[30m
RED			= \033[31m
GREEN		= \033[32m
YELLOW		= \033[33m
BLUE		= \033[34m
MAGENTA		= \033[35m
CYAN		= \033[36m
WHITE		= \033[37m
RESET		= \033[0m

R_MARK_UP	= $(MAGENTA)$(BOLD)
CA_MARK_UP	= $(GREEN)$(BOLD)

# ----------------------------------- messages ------------------------------- #
CUR_DIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
REMOVED := $(R_MARK_UP)REMOVED $(CYAN)%s$(MAGENTA) (%s) $(RESET)\n
CREATED := $(CA_MARK_UP)CREATED $(CYAN)%s$(GREEN) (%s) $(RESET)\n
