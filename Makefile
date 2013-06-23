CFLAGS = -Wall -Werror -g
BUILD_DIR = bin

# may have different implementaions in the future
INIT_IMPL = lib/init.c
#INIT_IMPL = lib/init_ll.c   #init_ll still not complete, work in progress
INODE_TABLE_IMPL = lib/inode_hashtable.c
PROXY_FS_OPS = lib/proxy_ops.c
UTILS = lib/utils.c

.PHONY: proxyfs tests clean

proxyfs: $(INODE_TABLE_IMPL) $(UTILS) $(PROXY_FS_OPS) $(INIT_IMPL) main.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o $(BUILD_DIR)/$@

tests: $(INODE_TABLE_IMPL) $(UTILS) tests/main.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o $(BUILD_DIR)/$@
	./$(BUILD_DIR)/tests

clean:
	rm -rf $(BUILD_DIR)/*
