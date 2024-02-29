# xinutest, spring 2024
A wrapper for Purdue University's cs-console that automatically uploads a compiled xinu binary to and powercycles a specified backend.

# disclaimer
I've tried my best to make this a mostly efficient and safe little program, but I've almost certainly missed a corner case somewhere that will break the program and maybe your shell/terminal.
I'm still an amateur with linux programming, so if you find a bug, feel free to message me with steps to reproduce and any other salient information and I'll do my best to track it down and squash it (or feel free to take a crack at the problem yourself and throw a PR my way)!

# mechanism of action
If you did Dr. Turkstra's shell project for 252, then you should understand a lot of what is happening here.
This program execls `cs-console` in a forked child process and does basic analysis of its output in the parent process.
Based on the analysis of `cs-console`'s output, the parent simulates terminal input using ioctl's TIOCSTI request.
The flow of information is approximately: `(terminal input)->(cs-console)->(xinutest)->(terminal output)/(terminal input)`

Everything else is pretty much fighting OS/library level buffering and implementing some basic I/O synchronization using `poll()`.
Roughly, the program performs these steps automatically:
  * confirm a successful connection to the specified backend
  * upload `xinu.xbin` to the backend, confirm successful upload
  * powercycle
  * relay all future `cs-console` output to `stdout`
    
A note on efficiency: I've done my best to make this program efficient with regards to system resources, but the nature of the way I've designed it means that it almost certainly throws away some speed and extra resources in exchange for its simplicity and "completeness" (i.e. at least it "works"!). If you've got suggestions for improvements or something I did that particularly bothers you, I would be glad to hear you out and learn more about how to solve this sort of problem professionally.

# how to use
1. Clone the repo
2. Compile xinutest using the provided Makefile (just run `make` in the root of the repo)
3. Add the line `export PATH=${PATH}:~/path/to/cloned/repo` to your .bashrc (usually located in ~/)
   - Not necessary but very useful, allows you to execute xinutest from any directory
4. Work in the `compile` directory of xinu as normal, and execute xinutest when you are ready with `xinutest <backend number>`
5. You will able to interact with `cs-console` as normal after `xinutest` automatically powercycles the backend

# lessons i think i learned
1. If you run into a difficult problem while linux programming, theres a good chance someone has solved it better than you could and theres at least one elegant way to reach it. If it exists, that solution is probably somewhere in the GNU coreutils or kernel (this problem was turning off full buffering of cs-console, see code).
2. Do not mix `poll()` and `FILE *` streams. I still don't understand the mechanism, but it appears to me  that `fread()`ing from a `poll()`'d file stream marks everything currently in the buffer as read (i.e. `poll()` returns 0 on future calls), which is not a desirable behavior (at least in this case).
3. Just because you can malloc doesn't mean you should: if you can think of a non-masochistic way to solve the problem on the stack, do that. It will save you a few system calls and the extra mental load of tracking the dynamically allocated memory.
4. If you start writing supporting functions for a supporting function, move that logical group to a new file. It keeps line count down, filenames descriptive, and helps you design and implement code in a modular fashion (e.g. `confirm_output()` originally lived as a `static` in xinutest.c).
