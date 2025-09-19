NAME			:=	Webserv

#	MAKEFLAGS will automatically apply the specified options (e.g., parallel execution) when 'make' is invoked
MAKEFLAGS		+=	-j

RM				:=	rm -rf
PRINT_NO_DIR	:=	--no-print-directory

#		CXXFLAGS for testing
CXX				:=	c++
CXXFLAGS		:=	-std=c++17
CXXFLAGS		+=	-Wall -Wextra
CXXFLAGS		+=	-Werror
CXXFLAGS		+=	-Wunreachable-code -Wpedantic -Wshadow -Wconversion -Wsign-conversion
CXXFLAGS		+=	-MMD -MP
# CXXFLAGS		+=	-g
CXXFLAGS		+=	-ggdb -fno-limit-debug-info -O0
#		Werror cannot go together with fsanitize, because fsanitize won't work correctly.
# CXXFLAGS		+=	-g -fsanitize=address

# ulimit -n, view the limit of fds. use ulimit -n <max_value> to set a new limit.
FD_LIMIT		:=	$(shell n=$$(ulimit -n); echo $$((n>1024?1024:n)))
CXXFLAGS		+=	-D FD_LIMIT=$(FD_LIMIT)
ifdef BUFFER
CXXFLAGS		+=	-D CLIENT_BUFFER_SIZE=$(BUFFER)	#make BUFFER=<value>
endif
ifdef TERMINAL_DEBUG
CXXFLAGS		+=	-D TERMINAL_DEBUG=true
endif

#		Directories
BUILD_DIR		:=	.build/
INCD			:=	include/
DOCKER_DIR		:=	testing/docker

#		SOURCE FILES
SRC_DIR			:=	src/

MAIN			:=	main.cpp
PARSE			:=	parsing.cpp		Aconfig.cpp			ConfigServer.cpp		Location.cpp
RUNSERVER		:=	RunServer.cpp	serverUtils.cpp		serverListenFD.cpp		cleanup.cpp   	clientConnection.cpp
REQUEST			:=	parsingReq.cpp	processReq.cpp		validation.cpp			Post.cpp		response.cpp			handleCgi.cpp
TRANSFER		:=	get.cpp 		post.cpp			toClient.cpp			cgi.cpp			chunk.cpp
MISC			:=	utils.cpp		Client.cpp			FileDescriptor.cpp		Logger.cpp		ErrorCodeClientException.cpp

SRC_ALL			:=	$(MAIN) $(addprefix parsing/, $(PARSE))		$(addprefix RunServer/, $(RUNSERVER))	$(addprefix request/, $(REQUEST))	\
					$(addprefix HandleTransfer/, $(TRANSFER))	$(MISC)

#		Find all .c files in the specified directories
SRCP			:=	$(addprefix $(SRC_DIR), $(SRC_ALL))

#		Generate object file names
OBJS 			:=	$(SRCP:%.cpp=$(BUILD_DIR)%.o)
#		Generate Dependency files
DEPS			:=	$(OBJS:.o=.d)

#		HEADERS
INCLUDE			:=	-I $(INCD)

BUILD			:=	$(CXX) $(INCLUDE) $(CXXFLAGS)

#		Remove these created files
DELETE			:=	*.out			**/*.out		.DS_Store	\
					**/.DS_Store	.dSYM/			**/.dSYM/

#		RECIPES
all:	$(NAME)

$(NAME): $(OBJS)
	$(BUILD) $(OBJS) -o $(NAME)
	@printf "$(CREATED)" $@ $(CUR_DIR)

$(BUILD_DIR)%.o: %.cpp
	@mkdir -p $(@D)
	$(BUILD) $(INCLUDE) -c $< -o $@

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

test2: all
	./$(NAME) testing/test1.conf

valgrind: all	# make TERMINAL_DEBUG=true valgrind
	valgrind -s --track-fds=yes --leak-check=full --show-leak-kinds=all ./$(NAME)

valgrind2: all
	valgrind -s --track-fds=yes --leak-check=full --show-leak-kinds=all ./$(NAME) testing/test1.conf

print-%:
	$(info $($*))

#		Include the dependency files
-include $(DEPS)

.PHONY: all clean fclean re test valgrind build up down docker_clean

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
