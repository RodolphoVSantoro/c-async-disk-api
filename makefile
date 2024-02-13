main=src/api.c
output=out
release_output=rinha-backend-2024
compiler=gcc
warn=-Wall -Wextra -Werror -pedantic
flags=-std=gnu99
debug=-fsanitize=address -g
release=-O3

ifndef PORT
override PORT = 9999
endif

default: $(main)
	$(compiler) -o $(output) $(flags) $(debug) $(warn) $(main)

release: $(main)
	$(compiler) -o $(release_output) $(flags) $(warn) $(release) $(main)

run:
	./$(output) $(PORT)

run-release:
	./$(release_output) $(PORT)

compResetDb:
	$(compiler) -o resetDb src/resetDb.c

resetDb: compResetDb
	./resetDb
