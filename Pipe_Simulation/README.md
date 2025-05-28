# Pipe Simulation - Shell Command Execution

This program simulates a shell executing the command:
`man ping | grep -A 7 -m 1 -e '-f' > output.txt`

## Features

- Demonstrates `fork()`, `execvp()`, and `pipe()` usage.
- Manages two child processes for `man` and `grep`.
- Redirects output to `output.txt`.
- Validates child process execution status.

## Notes

This is a focused simulation on Unix pipelines and redirection. It helps understand process forking and output piping between commands.
