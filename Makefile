all:
	$(CC) fsitest.c -o fsitest

.PHONY: clean
clean:
	rm fsitest
