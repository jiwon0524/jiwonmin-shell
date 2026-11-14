CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g -Iinclude
all: deu-shell

deu-shell: src/main.c src/plan.c include/plan.h
	$(CC) $(CFLAGS) src/main.c src/plan.c -o $@

test: tests/test_plan.c src/plan.c include/plan.h
	$(CC) $(CFLAGS) tests/test_plan.c src/plan.c -o test-plan
	./test-plan

clean:
	rm -f deu-shell test-plan
.PHONY: all test clean
