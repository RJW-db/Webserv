NAME			:=	Webserv

#	MAKEFLAGS will automatically apply the specified options (e.g., parallel execution) when 'make' is invoked
MAKEFLAGS		+=	-j

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
CCPFLAGS		+=	-ggdb -fno-limit-debug-info -O0
#		Werror cannot go together with fsanitize, because fsanitize won't work correctly.
# CCPFLAGS		+=	-g -fsanitize=address

#		Directories
BUILD_DIR		:=	.build/
INCD			:=	include/
DOCKER_DIR		:=	testing/docker

#		SOURCE FILES
SRC_DIR			:=	src/

MAIN			:=	main.cpp \
					RunServer/RunServer.cpp	RunServer/serverUtils.cpp	RunServer/clientConnection.cpp	RunServer/cleanup.cpp	\
					Server.cpp					serverListenFD.cpp		\
					parsing.cpp						ConfigServer.cpp			Aconfig.cpp		FileDescriptor.cpp	  Client.cpp			\
					request/parsingReq.cpp request/processReq.cpp request/response.cpp request/validation.cpp		request/Post.cpp			\
						loggingErrors.cpp					Location.cpp			\
					utils.cpp		HandleCgiTransfer.cpp  HandleTransferChunks.cpp handleGetTransfer.cpp handlePostTransfer.cpp handleCgi.cpp handleToClientTransfer.cpp\
					ErrorCodeClientException.cpp  Logger.cpp
# PARSE			:=	parse/parsing.cpp				parse/parse_utils.cpp

#		Find all .c files in the specified directories
SRCP			:=	$(addprefix $(SRC_DIR), $(MAIN))
# $(addprefix $(SRC_DIR)parsing/, $(PARSE))

#		Generate object file names
OBJS 			:=	$(SRCP:%.cpp=$(BUILD_DIR)%.o)
#		Generate Dependency files
DEPS			:=	$(OBJS:.o=.d)

#		HEADERS
INCS			:=	RunServer.hpp			Parsing.hpp		ConfigServer.hpp		Location.hpp		Aconfig.hpp		HttpRequest.hpp  utils.hpp ServerListenFD.hpp 	ErrorCodeClientException.hpp Logger.hpp
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
	valgrind -s --track-fds=yes ./$(NAME)

build:
	docker compose -f $(DOCKER_DIR)/docker-compose.yml build --build-arg HOST_IP=$(shell hostname -I | awk '{print $$1}')

up:
	docker compose -f $(DOCKER_DIR)/docker-compose.yml up -d

down:
	docker compose -f $(DOCKER_DIR)/docker-compose.yml down -v

docker_clean:
	docker system prune -af --volumes

enter:
	docker exec -it webserv bash

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
