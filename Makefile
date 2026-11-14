CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g -Iinclude
all: jiwonmin-shell

jiwonmin-shell: src/main.c src/plan.c src/runner.c src/commands.c src/commands_fs.c src/commands_text.c src/screenshot.c src/line_editor.c src/line_editor.h src/completion.c src/completion.h src/command_check.c src/command_check.h src/chain.c src/chain.h src/commands_internal.h src/screenshot.h include/commands.h include/plan.h include/runner.h
	$(CC) $(CFLAGS) src/main.c src/plan.c src/runner.c src/commands.c src/commands_fs.c src/commands_text.c src/screenshot.c src/line_editor.c src/completion.c src/command_check.c src/chain.c -o $@

test: jiwonmin-shell tests/test_plan.c src/plan.c include/plan.h
	$(CC) $(CFLAGS) tests/test_plan.c src/plan.c -o test-plan
	./test-plan
	sh tests/test_runner.sh
	sh tests/test_pipeline.sh
	sh tests/test_jobs.sh
	sh tests/test_commands.sh
	sh tests/test_regression.sh
	sh tests/test_screenshot.sh
	sh tests/test_help.sh
	sh tests/test_history.sh
	$(CC) $(CFLAGS) tests/test_completion.c src/completion.c -o test-completion
	./test-completion
	$(CC) $(CFLAGS) tests/test_command_check.c src/command_check.c -o test-command-check
	./test-command-check
	$(CC) $(CFLAGS) tests/test_chain.c src/chain.c -o test-chain
	./test-chain
	sh tests/test_chain_integration.sh
	sh tests/test_environment.sh

clean:
	rm -f jiwonmin-shell test-plan test-completion test-command-check test-chain
.PHONY: all test clean
