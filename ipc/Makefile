CC ?= gcc

all: fork wait alive block kill pipe serv clnt shmem trace

fork: fork.c
	$(CC) $< -o $@

wait: wait.c
	$(CC) $< -o $@

alive: alive.c
	$(CC) $< -o $@

block: block.c
	$(CC) $< -o $@

kill: kill.c
	$(CC) $< -o $@

pipe: pipe.c
	$(CC) $< -o $@

serv: serv.c
	$(CC) $< -o $@

clnt: clnt.c
	$(CC) $< -o $@

shmem: shmem.c
	$(CC) $< -o $@ -lrt

trace: trace.c
	$(CC) $< -o $@

clean:
	rm -f fork wait alive block kill pipe serv clnt shmem trace
