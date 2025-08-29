NAME			:=	Webserv

#	MAKEFLAGS will automatically apply the specified options (e.g., parallel execution) when 'make' is invoked
MAKEFLAGS		+=	-j

RM				:=	rm -rf
PRINT_NO_DIR	:=	--no-print-directory

#		CCPFLAGS for testing
COMPILER		:=	c++
CCPFLAGS		:=	-std=c++17
# CCPFLAGS		+=	-Wall -Wextra
# CCPFLAGS		+=	-Werror
# CCPFLAGS		+=	-Wunreachable-code -Wpedantic -Wshadow -Wconversion -Wsign-conversion
CCPFLAGS		+=	-MMD -MP
ifdef BUFFER
CCPFLAGS		+=	-D CLIENT_BUFFER_SIZE=$(BUFFER)	#make BUFFER=<value>
endif
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
					parsing/parsing.cpp		parsing/Aconfig.cpp			parsing/ConfigServer.cpp		parsing/Location.cpp						\
					RunServer/RunServer.cpp	RunServer/serverUtils.cpp	RunServer/serverListenFD.cpp	RunServer/clientConnection.cpp				\
					RunServer/cleanup.cpp																											\
					request/parsingReq.cpp	request/processReq.cpp		request/validation.cpp			request/Post.cpp							\
					request/response.cpp	request/handleCgi.cpp																					\
					HandleTransfer/get.cpp 	HandleTransfer/post.cpp		HandleTransfer/toClient.cpp		HandleTransfer/cgi.cpp						\
					HandleTransfer/chunk.cpp																										\
					utils.cpp		\
					FileDescriptor.cpp	  Client.cpp			\
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
INCLUDE			:=	-I $(INCD)

BUILD			:=	$(COMPILER) $(INCLUDE) $(CCPFLAGS)

#		Remove these created files
DELETE			:=	*.out			**/*.out			.DS_Store												\
					**/.DS_Store	.dSYM/				**/.dSYM/

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
