CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g -Iinclude
all: jiwonmin-shell

jiwonmin-shell: src/main.c src/plan.c src/runner.c src/commands.c src/commands_fs.c src/commands_text.c src/screenshot.c src/commands_internal.h src/screenshot.h include/commands.h include/plan.h include/runner.h
	$(CC) $(CFLAGS) src/main.c src/plan.c src/runner.c src/commands.c src/commands_fs.c src/commands_text.c src/screenshot.c -o $@

test: jiwonmin-shell tests/test_plan.c src/plan.c include/plan.h
	$(CC) $(CFLAGS) tests/test_plan.c src/plan.c -o test-plan
	./test-plan
	sh tests/test_runner.sh
	sh tests/test_pipeline.sh
	sh tests/test_jobs.sh
	sh tests/test_commands.sh
	sh tests/test_regression.sh
	sh tests/test_screenshot.sh

clean:
	rm -f jiwonmin-shell test-plan
.PHONY: all test clean
