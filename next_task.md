📅 Today's Progress Log — Custom Shell

Here's a clean markdown summary of what you built today.

---

# 🐚 Custom Shell — Day 1 Progress

## ✅ Features Implemented

| # | Feature | Status | Notes |
|---|---------|--------|-------|
| 1 | Built-in `cd` | ✅ Working | Uses `chdir()` directly in parent (not forked) |
| 2 | Built-in `exit` | ✅ Working | Clean termination |
| 3 | Built-in `help` | ✅ Working | Lists all features |
| 4 | External command execution | ✅ Working | Uses `fork()` + `execvp()` |
| 5 | Output redirection (`>`) | ✅ Working | `dup2()` + `O_WRONLY \| O_CREAT \| O_TRUNC` |
| 6 | Input redirection (`<`) | ✅ Working | `dup2()` + `O_RDONLY` |
| 7 | Single pipe (`cmd1 \| cmd2`) | ✅ Working | `pipe()` + two `fork()`s |
| 8 | Background processes (`&`) | ✅ Working | `SIGCHLD` handler reaps zombies |
| 9 | Job tracking | ✅ Working | Job table with pid, job_id, command |

---

## 🧠 Concepts Learned

### OS Concepts
- **Process creation**: `fork()`, `exec()`, `waitpid()`
- **File descriptors**: 0/1/2 std streams, `dup2()`, `open()` flag bitmask
- **Pipes**: kernel buffer + two fds, blocking semantics, EOF behavior
- **Signals**: `SIGCHLD` handler, `SA_RESTART`, `SA_NOCLDSTOP`
- **Async-signal-safety**: why `printf` in handler is unsafe, flag-based pattern
- **Zombie processes**: why `wait()` is mandatory, how `WNOHANG` solves it

### Systems Programming Patterns
- Per-process vs kernel-shared state (fd table layers)
- `signal()` vs `sigaction()` — why to prefer the latter
- `volatile sig_atomic_t` for handler-safe flags
- Reading a Linux `man` page properly (`man 7 signal-safety`)

---

## 🛠️ Key System Calls Used

```c
fork()          // create child process
execvp()        // replace process image
waitpid()       // reap children, block or non-blocking
pipe()          // create kernel IPC buffer
open()          // O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, O_APPEND
dup2()          // swap fd targets (the redirection primitive)
close()         // release fd
chdir()         // change working directory
getcwd()        // get current working directory
sigaction()     // register signal handler (modern, portable)
```

---

## 🧪 Test Commands Verified

```bash
>> ls
>> ls > out.txt
>> cat < out.txt
>> ls | grep txt
>> cat /etc/hosts | grep localhost > result.txt
>> sleep 5 &
[1] 12345
>> jobs
>> exit
```

---

## 🐛 Bugs Caught & Fixed Today

1. **Compile error**: `fopen()` used with `O_WRONLY` flags (typo — meant `open()`)
2. **Uninitialized `fd`**: assigned `open()` result to variable that was never declared in parent scope
3. **Wrong fd closed in parent**: `close(fd)` ran in parent where `fd` was uninitialized
4. **`r` passed by value but treated as out-parameter**: `parseRedirction` couldn't communicate type back to caller
5. **Empty `else if` branch**: forgot to call `executeRedirection` after parsing
6. **Pipe stripped `&`**: didn't propagate background flag to `executePiped`
7. **Redirected output swallowed by `&`**: `executeRedirection` always called `wait()`
8. **`input` mutated before storing**: job table lost the `&` because `parseInput` modified the string
9. **Two redundant background flags**: simplified to one
10. **No output for `ls | grep txt`**: turned out to be PATH issue — full path worked

---

## 📊 Code Stats

- **Lines of code**: ~200
- **Files**: 1 (`shell.c`)
- **Dependencies**: Standard libc only (`stdio.h`, `stdlib.h`, `unistd.h`, `sys/wait.h`, `signal.h`, `fcntl.h`, `string.h`, `errno.h`)
- **Compilation**: `gcc -Wall -Wextra -o shell shell.c`

---

## 🎯 Next Steps (Pending)

- [ ] `>>` append redirection
- [ ] `2>` stderr redirection
- [ ] Multiple pipes (`a | b | c`) via recursion
- [ ] Combined pipe + redirect (`cat f | grep x > out`)
- [ ] `jobs` built-in command (table already exists, just needs printer)
- [ ] `fg` / `bg` job control with `Ctrl-Z`
- [ ] Quoted strings (`"hello world"`)
- [ ] Subshells (`(cmd1; cmd2)`)

---

## 💡 Key Insights From Today

> "A pipe is just a kernel buffer with two ends. After fork, both processes have valid fds pointing to it. Closing the right fd in the right process is the entire trick."

> "Redirection isn't a feature. It's just swapping which file a file descriptor points to via `dup2()`."

> "Signal handlers don't run in the background — they're function pointers the kernel invokes at safe checkpoints. Your code resumes exactly where it left off."

---

## 📚 Resources Referenced

- `man 2 fork`, `man 2 execve`, `man 2 pipe`, `man 2 dup2`, `man 2 open`
- `man 7 signal-safety` — the async-signal-safe function list
- `man 2 sigaction` — preferred over `signal()`
- `man 2 waitpid` — `WNOHANG` flag

---

**Status: Working shell with foreground/background jobs, pipes, and redirection. Foundation solid for tackling Ctrl-Z job control next.** 🚀

---
