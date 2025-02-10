install:
	clib install --dev

test:
	@$(CC) test.c -std=c11 -fsanitize=address -g -I src -I deps -o $@
	@./$@

.PHONY: test
