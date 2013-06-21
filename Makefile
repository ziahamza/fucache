CFLAGS = -Wall -Werror

# may have different implementaions in the future
INIT_IMPL = lib/init.c
#INIT_IMPL = lib/init_ll.c   #init_ll still not complete, work in progress
INODE_TABLE_IMPL = lib/inode_hashtable.c
PROXY_FS_OPS = lib/proxy_ops.c

.PHONY: proxyfs tests clean

proxyfs: $(INODE_TABLE_IMPL) $(PROXY_FS_OPS) $(INIT_IMPL) main.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o bin/$@

tests: $(INODE_TABLE_IMPL) tests/inode_table_tests.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o bin/$@ && ./bin/tests

clean:
	rm -rf bin/*
