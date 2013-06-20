CFLAGS = -Wall -Werror

# may have different implementaions in the future
INIT_IMPL = init.c
# INIT_IMPL = init_ll.c
INODE_TABLE_IMPL = inode_hashtable.c
PROXY_FS_OPS = proxy_ops.c

proxyfs: $(INODE_TABLE_IMPL) $(PROXY_FS_OPS) $(INIT_IMPL) main.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o bin/$@

tests: $(INODE_TABLE_IMPL) inode_table_tests.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o bin/$@ && ./bin/tests
