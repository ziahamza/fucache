CFLAGS = -Wall -Werror

proxyfs: proxyfs.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o bin/$@
tests: inode_table.c inode_table_tests.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o bin/$@ && ./bin/tests
