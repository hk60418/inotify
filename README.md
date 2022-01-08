# Inotify

Simple program for monitoring file system events on Linux. 

## Getting Started

gcc inotify.c -o inotify

If you want to use this anywhere on you system, then you could copy the program to your PATH.

See inotify man page for available masks and complications of the interface.

### Examples

Monitor all file events in directories \.\./foo and \./bar:
```
inotify ../foo bar
```

Monitor only file creation and deleting in directories ./foo and /home/ubuntu/:
```
inotify --mask IN_CREATE --mask IN_DELETE foo /home/ubuntu/
```

Leave it running overnight:
```
nohup inotify foo /home/ubuntu/ &
```

Notice the nice log-like output format:
```
[hk60418@dev inotify]$ ./a.out . /home/hk60418
2018-04-16 21:36:12 /home/hk60418/foobar created
2018-04-16 21:36:30 ./foobar created
```

## Authors

* **Henri Kynsilehto** - [GitHub]\(https://github.com/hk60418\)

## Acknowledgments

* https://github.com/rvoicilas/inotify-tools - A bit more engineered inotify library.
