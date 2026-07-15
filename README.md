# Project 2: Linux Systems Programming

Four C programs exploring Linux process management, I/O, and multithreading with POSIX APIs: process pipelines (`fork`/`pipe`/`exec`), file copying (raw syscalls vs. buffered stdio), parallel prime counting (`pthread` + mutex), and multithreaded keyword search across files.

## Contents

| File | Question | What it does |
|---|---|---|
| `child_processes.c` | 1 | Recreates `ps aux \| grep root` using `fork()`, `pipe()`, `dup2()`, `execvp()` |
| `copy_system.c` | 2 | Copies a large file using raw `read()`/`write()` syscalls |
| `copy_stdio.c` | 2 | Copies a large file using buffered `fread()`/`fwrite()` |
| `multithreading.c` | 3 | Counts primes between 1 and 200,000 using 16 threads + mutex |
| `search.c` | 4 | Multithreaded keyword search across multiple text files |

## Prerequisites

- Linux (uses `<unistd.h>`, `<sys/wait.h>`, POSIX threads)
- `gcc`
- `strace` (optional, only needed to reproduce the syscall counts below)

## Build

```bash
gcc -O2 -o child_processes child_processes.c
gcc -O2 -o copy_system     copy_system.c
gcc -O2 -o copy_stdio      copy_stdio.c
gcc -O2 -o multithreading  multithreading.c -lpthread
gcc -O2 -o search          search.c -lpthread
```

## Run

**Question 1 — process pipeline**
```bash
./child_processes
```
Runs `ps aux | grep root` internally via two forked children connected by a pipe, writes the filtered result to `output.txt`, then the parent reads and prints a preview.

**Question 2 — file copy comparison**

Needs a large source file named `largefile.bin` in the working directory:
```bash
dd if=/dev/urandom of=largefile.bin bs=1M count=100
./copy_system   # writes copy_system.bin
./copy_stdio    # writes copy_stdio.bin
```

**Question 3 — parallel prime counting**
```bash
./multithreading
```
No arguments; always counts primes in `[1, 200000]` across 16 threads.

**Question 4 — multithreaded search**
```bash
./search <keyword> <output.txt> <file1.txt> <file2.txt> ... <number_of_threads>
# example:
./search root results.txt logs/a.txt logs/b.txt logs/c.txt 4
```
If `number_of_threads` exceeds the number of input files, the program caps it at the file count.

---

## Question 1: Process pipeline (`fork`, `pipe`, `dup2`, `execvp`)

### Explanation

The goal was to understand how processes communicate in Linux using system calls, by reimplementing `ps aux | grep root` in C instead of running it directly in a shell. The program creates two child processes with `fork()`. The first executes `ps aux`, the second executes `grep root`, and a pipe connects the first child's stdout to the second child's stdin — the same relationship the shell sets up for you with `|`. Rather than printing the result immediately, the filtered output is written to a file; the parent then opens that file, reads part of it, and prints it. `strace` was used to observe the actual system calls made during process creation, pipe communication, and file I/O.

### Implementation

A pipe is created first, giving a read end and a write end shared between the two children. The first child redirects its stdout to the pipe's write end with `dup2()` before calling `execvp("ps", ["ps", "aux"])`. The second child redirects its stdin to the pipe's read end and its stdout to an output file, then calls `execvp("grep", ["grep", "root"])`. Each child closes the pipe descriptors it doesn't need immediately after `dup2()`, and the parent closes both ends once the children are launched. The parent calls `wait()` twice to ensure both children finish — and therefore that the output file is fully written — before opening it and reading a preview with `read()`.

### Challenges

Understanding `dup2()`-based redirection was the main challenge: both children share the same pipe, but each uses a different end, and every process needs to close the descriptors it isn't using or the pipe never signals EOF correctly. The other challenge was avoiding a read-before-write race on the output file, solved by calling `wait()` for both children before the parent touches the file.

### Verified behavior

Compiled with `gcc -O2` and run directly (no `strace` overhead) — completed correctly, printing the `ps aux` lines containing `root` and a byte count read back from `output.txt`. Under `strace -f -c`, the run showed 2 `clone` calls (the two `fork()`s), 3 `dup2` calls, and 1 `pipe2` call, matching the implementation exactly.

---

## Question 2: File copy — raw syscalls vs. buffered stdio

### Explanation

The goal was to compare low-level system calls (`read()`/`write()`) against standard C library I/O (`fread()`/`fwrite()`) when copying a large file, and to understand how buffering affects the number of system calls and overall performance. Both versions use the same 4096-byte buffer size and were tested against a 100 MB file generated with `dd if=/dev/urandom`.

### Implementation

`copy_system.c` opens both files with `open()` and loops on `read()`/`write()`, each of which is a direct system call. `copy_stdio.c` opens both files with `fopen()` and loops on `fread()`/`fwrite()`, which go through the C library's internal `FILE*` buffer before hitting the kernel. Both versions time themselves with `clock()` around the copy loop.

### Measured results

Run on this machine, 100 MB random source file, 4096-byte buffer in both versions:

| Metric | `copy_system` (raw) | `copy_stdio` (buffered) |
|---|---|---|
| Execution time | 0.2185 s | 0.1932 s |
| `read` syscalls (strace -c) | 25,602 | 25,602 |
| Total syscalls (strace -c) | 51,241 | 51,243 |
| Output correctness | `diff` against source: identical | `diff` against source: identical |

### Performance analysis (honest version)

The buffered version was consistently a bit faster (~12% in this run), but **the syscall counts were essentially identical between the two**, because both programs use the *same* 4096-byte buffer size — `fread()`/`fwrite()` only reduce syscalls when the library buffer is *larger* than what the raw version reads per call. With matched buffer sizes, stdio's advantage here comes from smaller fixed per-call overhead (no `errno` handling, inlined memcpy paths) rather than fewer kernel transitions. To see the syscall-count gap the original hypothesis expects, `copy_system` would need a *smaller* buffer (e.g. 512 bytes) while `copy_stdio` keeps `BUFFER_SIZE` at 4096 — that widens the read-count difference because stdio still batches its underlying `read()`s at the larger internal buffer size.

This is a more accurate takeaway than "stdio always makes fewer syscalls" — it depends on the relationship between the two buffer sizes, not just which API is used.

---

## Question 3: Parallel prime counting (`pthread`, mutex)

### Explanation

The goal was to count primes in `[1, 200000]` using 16 POSIX threads instead of a single-threaded loop, dividing the range into 16 equal contiguous chunks (one per thread) and combining results into one shared counter, `totalPrimes`, protected by a `pthread_mutex_t`.

### Implementation

`isPrime()` trial-divides only up to `sqrt(n)`. Each thread receives a `{start, end}` range via `ThreadData`, accumulates its own `localCount` locally (avoiding a lock on every increment), and only takes the mutex once at the end to add its local count into the shared `totalPrimes`. The main thread creates all 16 threads with `pthread_create()`, joins them with `pthread_join()`, then destroys the mutex and prints the result.

### Challenges

The main risk was a race condition if all 16 threads incremented `totalPrimes` directly and concurrently. Accumulating locally and merging once per thread (rather than locking on every prime found) both fixes the race and minimizes lock contention. Dividing the range evenly required giving the leftover numbers (200000 doesn't divide evenly by 16 in this case it does, but the code still handles the general case) to the last thread.

### Verified results

Compiled and run: **17,984 primes** found between 1 and 200,000 — this matches the known count (the actual number of primes below 200,000 is 17,984), confirming the range partitioning and merge logic are correct with no off-by-one gaps or overlaps. `time` reported 388% CPU utilization (multiple cores active) and 0.011s wall-clock time.

---

## Question 4: Multithreaded file search (`pthread`, mutex, CLI args)

### Explanation

The goal was to search for a keyword across multiple text files in parallel, one thread per file, writing all results to a single shared output file, and to compare execution time across different thread counts.

### Implementation

Arguments are parsed as `keyword`, `output file`, one or more input files, and a trailing thread count; if the requested thread count exceeds the number of files, it's capped to the file count. Each thread opens its assigned file, tokenizes it with `fscanf("%99s", ...)`, and compares each token against the keyword with `strcmp()`. Threads are launched and joined in batches sized to the thread count (so at most `threadCount` threads run concurrently even with more files than threads). Writing to the shared output file is protected by a mutex so concurrent `fprintf()` calls from different threads can't interleave.

### Measured results

6 test files (~128 KB each, ~2,000 occurrences of the keyword `root` per file). The assignment asks for three specific runs, so here they are with what each one means in plain terms:

| Threads requested | Why this number | Threads actually used | Execution time |
|---|---|---|---|
| 2 | The 2-thread case the assignment asks for | 2 | 0.0172 s |
| 6 | Max threads — one thread per file, since there are 6 files | 6 | 0.0219 s |
| 8 | Average CPU cores — `nproc` reports 8 cores on this machine | 6 (capped, only 6 files to hand out) | 0.0187 s |

All three runs gave the exact same occurrence counts per file, so asking for more threads never changed the answer, only the speed. With files this small, the actual search barely takes any time, so most of what you're measuring is the overhead of creating and joining threads — that's why 6 threads isn't clearly faster than 2, and why asking for 8 threads doesn't help once it gets capped down to 6. You'd need much bigger files before adding threads actually pays off.

### Challenges

Coordinating shared output access was solved with a mutex around the write section only, so the (slower) file-reading and searching work still happens fully in parallel outside the lock. Capping thread count to file count avoids spawning idle threads with nothing to search.
