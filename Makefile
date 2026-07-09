NAME = ft_ping

SRC =	srcs/main.c \
		srcs/init.c \
		srcs/message.c

OBJ = $(SRC:.c=.o)

all : ${OBJ}
	@cc ${OBJ} -o ${NAME}

clean :
	@rm -f ${OBJ} ${OBJ_BONUS}

fclean : clean
	@rm -f ${NAME}

re : fclean all

.PHONY : all clean fclean re