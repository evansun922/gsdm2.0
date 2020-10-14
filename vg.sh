valgrind --leak-check=full --track-origins=yes --show-reachable=yes --track-fds=yes --log-file=vg.log $@;
cat vg.log

