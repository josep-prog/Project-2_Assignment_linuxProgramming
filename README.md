# **Project 2: Linux Systems Programming**

**Repository :** [Project-2\_Assignment\_linuxProgramming](https://github.com/josep-prog/Project-2_Assignment_linuxProgramming.git) 

**Date :** 17th July 2026

| Name | Link |
| :---- | :---- |
| Question 1: fork, execvp , pipe | [child\_processes.c](https://github.com/josep-prog/Project-2_Assignment_linuxProgramming/blob/main/child_processes.c)  |
| Question 2 : low-level system calls | standard I/O functions | [copy\_system.c](https://github.com/josep-prog/Project-2_Assignment_linuxProgramming/blob/main/copy_system.c)  |
|  | [copy\_stdio.c](https://github.com/josep-prog/Project-2_Assignment_linuxProgramming/blob/main/copy_stdio.c)  |
| Question 3: prime numbers between 1 and 200,000 using 16 threads | [multithreading.c](https://github.com/josep-prog/Project-2_Assignment_linuxProgramming/blob/main/multithreading.c)  |
| Question 4: multithreaded keyword search across files | [search.c](https://github.com/josep-prog/Project-2_Assignment_linuxProgramming/blob/main/search.c)  |

## 

**Question 1 : process pipeline**

The objective of this assignment was to help me understand how processes communicate in Linux using system calls. Since I am learning process programming for the first time, I wanted to understand what happens behind the scenes when a command like ps aux | grep root is executed. Instead of running the command directly in the terminal, I wrote a C program that creates the pipeline using fork(), pipe(), dup2(), and execvp(). I also used **strace** to observe the system calls used during execution.

## **Implementation**

I first created a pipe so that two child processes could communicate. The first child process redirects its standard output to the pipe and executes ps aux using execvp(). The second child redirects its standard input to the pipe, redirects its standard output to **output.txt**, and executes grep root. This allows the output of ps aux to be filtered and saved into the file. The parent process closes the pipe, waits for both children to finish using wait(), then opens the file, reads part of its contents, and displays it on the screen.

## **Challenges**

The main challenge was understanding how dup2() redirects input and output. At first, I found it confusing how one process writes to the pipe while another reads from it. I also learned that every process must close the pipe ends it does not use. Another challenge was making sure the parent waited for both child processes before reading the output file, so that all the data had already been written.

## **Execution Behavior**

The program compiled and ran successfully. It created two child processes, executed ps aux and grep root, saved the filtered output into **output.txt**, and displayed part of the file.

**strace analysis.** I traced the program with:

```
strace -f -tt -e trace=clone,execve,pipe,pipe2,dup2,openat,open,read,write,close,wait4 -o strace_logs/q1_pipeline_excerpt.txt ./child_processes
```

The filtered log (full copy in [`strace_logs/q1_pipeline_excerpt.txt`](strace_logs/q1_pipeline_excerpt.txt)) shows the exact sequence the code produces:

```
201459 execve("./child_processes", ...) = 0
201459 pipe2([3, 4], 0) = 0
201459 clone(...) = 201460        # first child: will run ps aux
201459 clone(...) = 201461        # second child: will run grep root
201461 openat(AT_FDCWD, "output.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 5
201460 dup2(4, 1) = 1             # child 1: stdout -> pipe write end
201461 dup2(3, 0) = 0             # child 2: stdin  -> pipe read end
201459 wait4(-1 <unfinished ...>  # parent blocks here
201461 dup2(5, 1) = 1             # child 2: stdout -> output.txt
201460 execve("/usr/bin/ps",   ["ps", "aux"], ...)   = 0
201461 execve("/usr/bin/grep", ["grep", "root"], ...) = 0
201459 <... wait4 resumed>) = 201460
201459 wait4(-1, NULL, 0, NULL) = 201461
201459 openat(AT_FDCWD, "output.txt", O_RDONLY) = 3
```

This is a direct trace of the program's own logic: `fork()` shows up as two `clone()` calls (PIDs 201460 and 201461) made by the parent (201459); `pipe()` appears as `pipe2([3,4], 0)` creating the read/write ends *before* either child exists, which is exactly why both children inherit the same fds; each child's `dup2()` call rewires its stdin/stdout onto the pipe (or onto `output.txt` for the grep child) before it calls `execvp()`, which is why redirection has to happen before `execve()` and not after; and the parent's two `wait4()` calls block until both children exit, guaranteeing `output.txt` is fully written before the parent's own `openat(..., O_RDONLY)` reads it back — confirming the ordering my "Challenges" section above described.

A full `strace -f -c ./child_processes` run (saved in [`strace_logs/q1_syscall_summary.txt`](strace_logs/q1_syscall_summary.txt)) counted **2 `clone`, 1 `pipe2`, 3 `dup2`, 3 `execve`, and 2 `wait4`** calls — matching one `pipe()`/`fork()` pair per child and one redirect per fd being changed, plus one `execve` for the shell-less program itself, `ps`, and `grep`. The large `read`/`openat`/`close` counts in that summary (1174/1380/936 calls) come from `ps aux` itself scanning every process's `/proc/<pid>/stat` entry, not from the pipeline code — this is expected `ps` behavior, not overhead introduced by my program.

## gcc \-o child\_processes child\_processes.c

## ./child\_processes

Runs ps aux | grep root internally via two forked children connected by a pipe, writes the filtered result to output.txt, then the parent reads and prints a preview.

## 

## 

## 

## 

## 

## 

## 

## **Question 2: File Copy Using System Calls and Standard I/O**

The goal of this question was to compare two different ways of copying a large file in C. The first version uses low-level Linux system calls (read() and write()), while the second version uses the standard C library functions (fread() and fwrite()). Since I am learning file handling for the first time, this assignment helped me understand the difference between these two approaches. I also used **strace** to compare the number of system calls and measured the execution time of each program while copying a 100 MB file.

## **Implementation**

I wrote two separate programs because each one demonstrates a different method of copying a file. The first program, **copy\_system.c**, opens the source and destination files using open(), copies the file with read() and write(), and closes the files using close(). The second program, **copy\_stdio.c**, performs the same task using fopen(), fread(), fwrite(), and fclose(). Both programs use the same buffer size of **4096 bytes** and measure their execution time using clock().

## **Challenges**

The main challenge was understanding why I needed two separate programs instead of combining everything into one file. I learned that the purpose of the assignment is to compare two different file-copy methods fairly. Another challenge was understanding why the strace results showed almost the same number of system calls. After reviewing the results, I realized that both programs use the same 4096-byte buffer, so they make almost the same number of read() operations. This showed me that using fread() and fwrite() does not always reduce the number of system calls. The difference depends on the buffer size being used.

## **Execution Behavior**

gcc  \-o copy\_system     copy\_system.c

gcc  \-o copy\_stdio      copy\_stdio.c

Needs a large source file named largefile.bin in the working directory:

dd if=/dev/urandom of=largefile.bin bs=1M count=100

./copy\_system   \# writes copy\_system.bin

./copy\_stdio    \# writes copy\_stdio.bin

Both programs successfully copied the 100 MB file, and I confirmed that the copied files were identical to the original using diff. The execution time showed that the standard I/O version was slightly faster (**0.1932 seconds**) than the system call version (**0.2185 seconds**), though I noticed on repeat runs the gap is small and inconsistent — with disk caching involved, this timing difference is noisy and shouldn't be read as a reliable performance verdict from a single run.

**strace analysis.** To actually verify the "same number of system calls" claim instead of just asserting it, I ran:

```
strace -c -o strace_logs/q2_copy_system_summary.txt ./copy_system
strace -c -o strace_logs/q2_copy_stdio_summary.txt  ./copy_stdio
```

| syscall | copy\_system (read/write) | copy\_stdio (fread/fwrite) |
| :---- | :---- | :---- |
| read | 25602 | 25602 |
| write | 25601 | 25601 |
| openat | 4 | 4 |
| close | 4 | 4 |
| total syscalls | 25626 | 25630 |

(Full per-call breakdowns are in [`strace_logs/q2_copy_system_summary.txt`](strace_logs/q2_copy_system_summary.txt) and [`strace_logs/q2_copy_stdio_summary.txt`](strace_logs/q2_copy_stdio_summary.txt).) 100 MB copied in 4096-byte chunks is 100×1024×1024 / 4096 = 25600 chunks, plus one extra empty-return `read` at EOF, which lines up exactly with the 25601/25602 counts — proving the two programs hit the kernel the same number of times. This is concrete evidence, not just an inference: `fread()`/`fwrite()` are a *user-space* buffer sitting on top of the same `read()`/`write()` syscalls, so when both programs use identical 4096-byte buffers, stdio adds no syscall-count benefit at all. The benefit stdio usually provides only shows up when the stdio buffer is left at its own default (typically larger than the app's manual buffer, e.g. `BUFSIZ` internals) or when the caller uses a small read/write size while stdio coalesces it internally — neither of which applies here since I sized both buffers at 4096 bytes on purpose. This confirms the buffer-size explanation in the Challenges section above with actual numbers instead of a guess.

## 

## 

## **Question 3: Parallel prime counting (pthread, mutex)**

### 

The objective of this assignment was to learn how to use multiple threads in C to solve a problem faster. Since I am learning multithreading for the first time, this assignment helped me understand how POSIX threads (pthread) work, how a large task can be divided among several threads, and why synchronization is important when multiple threads access the same shared variable.

## **Implementation**

I created a program that uses **16 POSIX threads** to count the prime numbers between **1 and 200,000**. I divided the range equally so that each thread processes its own section of numbers. Each thread counts the prime numbers in its assigned range and stores the result in a local variable. After finishing, the thread updates the shared variable totalPrimes. To prevent multiple threads from modifying the shared variable at the same time, I used a pthread\_mutex\_t mutex. Each thread locks the mutex before updating the counter and unlocks it immediately after the update. Finally, the main thread waits for all 16 threads to finish using pthread\_join() before printing the total number of prime numbers.

## **Challenges**

The main challenge was understanding why a mutex was necessary. At first, I thought each thread could update the shared counter directly. However, I learned that if multiple threads write to the same variable at the same time, the final result may be incorrect due to a race condition. Using pthread\_mutex\_lock() and pthread\_mutex\_unlock() ensured that only one thread updated the shared counter at a time. I also had to make sure that the workload was divided evenly so every thread processed a different range of numbers.

## **Execution Behavior**

./multithreading

No arguments; always counts primes in \[1, 200000\] across 16 threads.

gcc \-O2 \-pthread \-o multithreading multithreading.c

The program compiled and executed successfully. All **16 threads** were created, each processed its assigned range, and the final result was printed only after all threads completed their work. The output displayed the synchronized total number of prime numbers between **1 and 200,000**, confirming that the threads worked correctly and that the mutex successfully protected the shared counter from race conditions. This assignment helped me understand how multithreading can improve performance while synchronization ensures that shared data remains correct.

## 

## 

## **Question 4: Multithreaded file search (pthread, mutex, CLI args)**

### 

The goal was to search for a keyword across multiple text files in parallel, one thread per file, writing all results to a single shared output file, and to compare execution time across different thread counts.

### **Implementation**

For this one I wrote a program that accepts a keyword, an output file, one or more input files, and the number of threads as command-line arguments. Each requested thread is started, and instead of statically pre-assigning one fixed file per thread, every thread repeatedly pulls the next unclaimed file from a shared index (protected by a mutex) until no files are left. Each thread opens its claimed file using fopen(), reads each word with fscanf(), compares it with the keyword using strcmp(), and counts the number of matches, then writes its result to a shared output file using fprintf() (protected by a second mutex so writes never interleave). The program measures the execution time using clock() and repeats the test with different thread counts.

**Fixing a thread-capping bug.** My first version capped `threadCount` down to the number of files whenever more threads were requested than files existed (e.g. requesting 8 threads with only 6 files silently became 6 threads) — which meant two of the three required test configurations ended up creating the *same* actual number of threads, defeating the point of comparing them. I fixed this by switching from "one thread pre-assigned to one file" to the shared-index pull model described above: now exactly `threadCount` OS threads are created and joined every time, no matter how many files there are. If there are more threads than files, the extra threads simply find the shared index already exhausted and exit immediately — but they are still genuinely created, so their creation/join cost is really paid and really measured, instead of being silently designed away.

I verified this with strace: `strace -f -c -e trace=clone ./search root out.txt f1.txt f2.txt f3.txt f4.txt f5.txt f6.txt 8` (6 files, 8 threads requested) reports **8 `clone3` calls** — proof that all 8 threads are now actually created rather than being capped to 6 (full output in [`strace_logs/q4_thread_count_verification.txt`](strace_logs/q4_thread_count_verification.txt)).

### **Measured results**

./search \<keyword\> \<output.txt\> \<file1.txt\> \<file2.txt\> ... \<number\_of\_threads\>

For example : 

./search root results.txt logs/a.txt logs/b.txt logs/c.txt 4

gcc \-O2 \-pthread \-o search search.c

For testing, I used six text files, each containing 1,177 occurrences of the keyword "root." I then ran the program using the three thread configurations required by the assignment and compared the execution times: 

| Threads requested | Why this number | Threads actually created (verified via strace) | Execution time |
| :---- | :---- | :---- | :---- |
| 2 | The 2-thread case the assignment asks for | 2 | 0.0028 s |
| 6 | Max threads one thread per file, since there are 6 files | 6 | 0.0048 s |
| 8 | Average CPU cores  nproc reports 8 cores on this machine | 8 (no longer capped — 2 of them find no file left and exit right away) | 0.0053 s |

All three runs gave the exact same occurrence counts per file, so asking for more threads never changed the answer, only the speed. With files this small, the actual search barely takes any time, so most of what you're measuring is the overhead of creating and joining threads — that's why more threads doesn't make things faster here, and in fact 8 threads is slightly slower than 6 or 2 purely from paying extra thread-creation/join cost for threads that end up doing no work. You'd need much bigger files, or many more files than threads, before adding threads actually pays off.

## **Challenges**

The main challenge was understanding how to safely write to one output file from multiple threads. Without synchronization, two threads could write at the same time and produce incorrect or mixed output. I solved this by protecting the writing section with pthread\_mutex\_lock() and pthread\_mutex\_unlock(). Another challenge was handling cases where the requested number of threads was greater than the number of input files. I solved this by limiting the number of threads to the number of available files so that every thread had useful work to do.