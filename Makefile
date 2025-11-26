# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ltouret <ltouret@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/03/05 13:00:20 by ltouret           #+#    #+#              #
#    Updated: 2025/03/05 01:24:08 by ltouret          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = icmp_chat

SRCS = client.c

OBJS = ${SRCS:.c=.o}

CC		= cc
RM		= rm -f

.c.o:
		${CC} -c $< -o ${<:.c=.o}

$(NAME): ${OBJS}
		${CC} ${OBJS} -o ${NAME}

all:	${NAME}

clean:
		${RM} ${OBJS}

fclean:	clean
		${RM} ${NAME}

re:		fclean all

.PHONY: all clean fclean re