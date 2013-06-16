# fucache
The plan is to write a generic FUSE filesystem that caches the static files and resources specially for the heuristics related to work load.

## current status
visit [http://ziahamza.wordpress.com](http://ziahamza.wordpress.com)

## updates
visit http://ziahamza.wordpress.com

## current status
fiddling around with fuse, learning the high and low level api
Some notes exist in the notes folder which I would increase as I explore
more fuse api

Right now a simple and stupid proxy file system exists for testing and learning,
it basically mounts the root filesystem on a mountpoint and all the calls are
proxied through fuse,

## build instructions
to build proxyfs, simply run

    make proxyfs

for usage, run
    ./proxyfs --help






