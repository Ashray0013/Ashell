# Custom Shell

A shell is a special program integrated into an operating system that provides humans with an interface to interact with the computer.

## Features Implemented

1. **Input Handling** – Reads user commands from stdin.
2. **Parsing Input** – Splits commands into arguments.
3. **Piping & Redirection** – Uses file descriptors to connect processes and redirect streams.
4. **Built-in Command Check** – Supports `cd`, `exit`, and `help`.
5. **Execution** – Runs external commands using `execvp`.
6. **Waiting** – Parent waits for child processes to finish.
7. **Looping** – Continues until `exit` is called.

## The Three Standard Streams

| FD | Name   | Default   | Redirection Operator |
|----|--------|-----------|----------------------|
| 0  | stdin  | Keyboard  | `<` (input)          |
| 1  | stdout | Terminal  | `>` (output)         |
| 2  | stderr | Terminal  | `2>` (error output)  |

## Usage Examples

- Run a command:
```gcc runcmd.c -o runcmd && ./runcmd
```

- Pipe commands:
```bash
ls | grep txt
```

- Redirect output:
```bash
ls > out.txt
```

- Redirect input:
```bash
sort < input.txt
```

- `cd <dir>`
- `help`
- `exit`

## How It Works

The shell continuously prompts for input.

Parses the command into arguments.

Checks for built-in commands.

Handles piping (|) and redirection (<, >).

Executes external commands via fork() + execvp().

Waits for child processes before prompting again.

## Notes

This is a simplified shell for learning purposes.

Error handling is basic; production shells are far more complex.

Extendable to support features like >> (append redirection), multiple pipes, and background processes (&).