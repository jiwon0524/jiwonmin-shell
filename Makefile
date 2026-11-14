CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g -Iinclude
all: deu-shell

deu-shell: src/main.c src/plan.c src/runner.c include/plan.h include/runner.h
	$(CC) $(CFLAGS) src/main.c src/plan.c src/runner.c -o $@

test: deu-shell tests/test_plan.c src/plan.c include/plan.h
	$(CC) $(CFLAGS) tests/test_plan.c src/plan.c -o test-plan
	./test-plan
	sh tests/test_runner.sh
	sh tests/test_pipeline.sh
	sh tests/test_jobs.sh

clean:
	rm -f deu-shell test-plan
.PHONY: all test clean
