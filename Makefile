CC = clang
CFLAGS = -Wall -Werror

proxyfs: proxyfs.c
	$(CC) $(CFLAGS) $^ `pkg-config fuse --cflags --libs` -o $@

