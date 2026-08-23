🚀 Next Steps to Level Up Your Shell

Congrats — you've built a working shell. Now comes the part where it actually gets interesting. Here's a roadmap from "toy project" to "real shell."

---

## 🎯 Level 1 — Polish What You Have (1-2 days each)

### 1. Background processes (`&`)
Right now your shell blocks on every command. Make `sleep 5 &` return immediately.
- Don't call `wait()` for background commands
- Install a `SIGCHLD` handler with `WNOHANG` to reap zombies
- Print job number when launching: `[1] 1234`

### 2. Append redirect (`>>`)
You're 2 lines away from this:
```c
fd = open(args2[0], O_WRONLY | O_CREAT | O_APPEND, 0644);
//                              ^^^^^^^^ instead of O_TRUNC
```

### 3. Error redirect (`2>`)
Same pattern, just dup2 onto `STDERR_FILENO` (which is `2`) instead of `STDOUT_FILENO`.

### 4. Combine pipe + redirect
The real test: `cat file | grep foo > out.txt`
- This requires parsing in the right order: handle `>` first (innermost), then `|`
- Or build a proper pipeline where the last stage gets stdout redirected

### 5. Multiple pipes (`a | b | c | d`)
Your current `parsePipe` only handles one. Generalize by **recursing**:
```c
void executePipeline(char *input) {
    char *pipePos = strchr(input, '|');
    if (!pipePos) {
        // base case: just one command, run it
        executeSimple(input);
        return;
    }
    *pipePos = '\0';
    char *right = pipePos + 1;
    while (*right == ' ') right++;
    
    // fork: left side writes to pipe, right side is a recursive pipeline
    int fd[2]; pipe(fd);
    if (fork() == 0) {
        dup2(fd[1], STDOUT_FILENO); close(fd[0]); close(fd[1]);
        executePipeline(input);   // recurse on left
    }
    if (fork() == 0) {
        dup2(fd[0], STDIN_FILENO); close(fd[0]); close(fd[1]);
        executePipeline(right);   // recurse on right
    }
    close(fd[0]); close(fd[1]);
    wait(NULL); wait(NULL);
}
```

This recursion is genuinely elegant and worth getting.

---

## 🎯 Level 2 — Real Shell Features (3-5 days each)

### 6. Quoted strings (`"hello world"`)
This breaks your `strtok` parser. You'll need a custom tokenizer that respects quotes.

### 7. Job control (`jobs`, `fg`, `bg`, `Ctrl-Z`)
- Track background jobs in a struct array: `pid`, `status`, `command`
- `Ctrl-Z` sends `SIGTSTP` — your shell needs to handle it and stop the foreground job
- `fg` resumes it with `SIGCONT`
- This requires `tcsetpgrp()` to manage which process group owns the terminal

### 8. Globbing (`*`, `?`)
When user types `ls *.c`, expand `*.c` to actual filenames before exec.
- Use `glob()` from `<glob.h>` — POSIX does the work for you
- Or write it yourself to learn how shells do it

### 9. Environment variables (`$HOME`, `$PATH`)
```bash
>> echo $HOME
```
- Before exec, walk the args and expand `$VAR` to `getenv("VAR")`

### 10. Command history (`↑` arrow, `!`)
- Store last N commands in a circular buffer
- For arrow keys, switch to **raw mode** (`termios`) instead of line-buffered input
- This is a big jump — you'd basically be rewriting your input loop

---

## 🎯 Level 3 — Hardcore OS Stuff (1-2 weeks each)

### 11. Tab completion
Requires raw mode + parsing the current word + filesystem traversal.

### 12. Redirection of file descriptors (`2>&1`, `>&`)
This is where real shell parsing gets tricky. You'd need a proper tokenizer with redirection operators as separate tokens.

### 13. Shell scripts
Execute a file line by line:
```bash
>> source myscript.sh
```
- Just read each line and feed it back into your parser
- Add `#` as a comment character

### 14. Pipes with built-in commands
Currently `cd | ls` fails because `cd` is a built-in (doesn't fork). Real shells run built-ins as part of the pipeline if needed. Complex topic.

---

## 🎯 Level 4 — Tools to Build ON TOP of Your Shell Knowledge

Once your shell feels solid, you've earned the right to build things that *use* these primitives:

### 15. **A mini `make` / build system**
Re-implement `make`'s dependency tracking. You'll deeply understand process spawning, file timestamps, and parallel execution (`make -j4`).

### 16. **A process supervisor**
Like a tiny `systemd` or ` supervisord`:
- Spawn daemons
- Restart on crash
- Log to files
- Read config file
You'll use `fork`, `setsid`, signal handlers, file descriptors, sockets — everything.

### 17. **A remote shell over TCP**
Combine your shell + raw sockets:
```bash
>> nc 192.168.1.5 4444
```
- Server runs commands and sends output back
- Add basic auth (token check)
- This is the foundation of SSH (poorly)

### 18. **A container runtime**
Like a tiny `docker run`:
- `unshare()` for PID namespace, mount namespace, network namespace
- `chroot()` for filesystem isolation
- `clone()` with flags
- You'll learn Linux containers from the inside

### 19. **An HTTP client with persistent connections**
Build `curl`:
- DNS resolution with `getaddrinfo`
- TCP connection
- HTTP request construction
- Response parsing
- HTTPS via OpenSSL

### 20. **A line-based text editor (like `ed` or `sed`)**
- Read files, edit in memory
- Commands like `5,10d` (delete lines 5-10), `s/foo/bar/` (substitute)
- You'll deeply understand buffering and file I/O

---

## 🎯 My Recommended Order

Pick based on what excites you:

```
Want to learn signals + processes?    → Background processes + Ctrl-Z
Want to learn recursion?              → Multiple pipes
Want to learn parsing?                → Quoted strings
Want to learn terminals?              → Raw mode + arrow keys + tab completion
Want to learn networking?             → Remote shell over TCP
Want to learn containers?             → Mini docker
Want to learn everything at once?     → Process supervisor
```

---

## 💡 One Piece of Career Advice

After you finish 2-3 of these projects, **put them on GitHub with clean READMEs**. A working shell with background processes, pipes, and redirection is genuinely impressive to interviewers. It's the kind of project that demonstrates you actually understand how Unix works under the hood — not just how to use it.

If you want, I can walk you through implementing any of these step by step. **Background processes with SIGCHLD** is probably the best next move — it teaches signals, race conditions, and process management all at once.
