main=src/api.c
output=out
compiler=gcc
warn=-Wall -Wextra -Werror -pedantic
flags=-std=c99

ifndef PORT
override PORT = 9999
endif

default : $(main)
	$(compiler) -o $(output) $(flags) $(warn) $(main)

run: $(output)
	./$(output) $(PORT)
