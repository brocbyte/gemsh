# gemsh
gemsh is a simple bash-like implementation of a shell in C. Written as a project for Operating System course.
It demonstrates the basics of a regular shell pipeline: read, parse, fork, exec.
***
Supported features:
+ multiple-line commands
+ stdin/stdout/stderr redirection
+ childish job-control: foreground/background process group management, some simple builtins
+ piping
+ signal handling (ctrl+c, ctrl+z)
+ only builtins are: `cd`,`jobs`, `fg`, `bg`
***
Also for now it has many limitations:
+ No quoting arguments
+ No globbing
+ No `history` builtin, arrow keys rip :(
