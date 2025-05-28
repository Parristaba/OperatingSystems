# Command Line Interface (CLI) Simulator

This C project simulates a command-line interpreter that executes commands from a file, handles pipes and I/O redirection, and manages background jobs.

## Features

- Parses and executes commands from `commands.txt`.
- Supports input/output redirection (`<`, `>`).
- Supports background processes (`&`) and wait command.
- Uses pipes and threads to manage parallel process outputs.
- Logs parsed command information into `parse.txt`.

## Notes

The project demonstrates low-level Unix process control, inter-process communication, and threading with `pthread`.
