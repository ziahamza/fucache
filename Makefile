CFLAGS = -Werror -Wall -g
BUILD_DIR = bin

# select one of the init implementations, INIT_LL is where the current development is happening
INIT_IMPL = lib/init.c
#INIT_IMPL = lib/init_ll.c   # semi usable for simple dir transversal, but still highly unstable and contains deadlocks

INODE_TABLE_IMPL = lib/inode_hashtable.c
PROXY_FS_OPS = lib/proxy_ops.c
# PROXY_FS_OPS = lib/proxy_mmap_ops.c # caches the file descriptors permantly for now and only works correctly in single thread

UTILS = lib/utils.c

.PHONY: proxyfs tests clean

proxyfs: $(INODE_TABLE_IMPL) $(UTILS) $(PROXY_FS_OPS) $(INIT_IMPL) main.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o $(BUILD_DIR)/$@

tests: $(INODE_TABLE_IMPL) $(UTILS) tests/main.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o $(BUILD_DIR)/$@
	./$(BUILD_DIR)/tests

clean:
	rm -rf $(BUILD_DIR)/*
