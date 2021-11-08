# 6-shell-2

Apart from my previous Shell 1 program, this new program contains the following changes. 

To begin with, I first handled signals in the main and within my fork, using the signal() system call. Moreover, before my call to execv(), I used signal handlers to set the behavior of the previously ignored signals to SIG_DFL, so that the child process could accept and act on signals. 

Following this, after I parse, I added an additional 'if' statement checking for the existence of an ampersand (&), which indicates that a job should be run in the background. I set it up to be a global variable, which I initialize and later reset to be zero, and set to 1 if it exists in the command line. I also set up the job list to be a global variable, which I initialize before my REPL.

Then, right after forking, I called the following functions: setpgid, and tcsetpgrp(). Setpgid sets the process group ID of the child process to the child process' pid, and then tcsetpgrp gets the process group ID of the current process. I call setpgid right after forking, and then, if there is NO ampersand (it is a foreground process), I call tcsetpgrp to give terminal control to the job (since background jobs shouldn't have terminal control.

Outside of the child process (in the parent process), I then first check if there is an ampersand. If there is one (ie, it is a background job), I add the job to the joblist (using a helper function) and print out the appropriate message. Otherwise, if it is a foreground jo, I call waitpid, tcsetpgrp, and check the following flags against the status: WIFEXITED (if true, I remove the job), WIFSTOPPED (if true, it is suspended, and I add the job), and WIFSIGNALED (if true, I print out a message). 

I also create a helper to handle reaping background processes - reap_helper() - which I call at the very start of my REPL. It essentially checks all background processes using the flags and statuses as mentioned previously, so that any zombie background jobs can be terminated, so that space in the system can be freed up. 

Then, I handle the following commands: fg, bg, and jobs. Within my built_ins() function, I add three additional if statements which check the filepath (ie., argv[0] excluding redirects) against the commands. If jobs is a command, I call the jobs function from jobs.c, and pass in the global joblist variable. If fg was the command, I then call a helper function I created, fg_helper(), which parses to get the jid, and gives the job terminal control, calls kill(), then waitpid, and checks the status passed into waitpid against various flags to ensure the job should be removed if it is terminated / terminated normally, and/or updated if suspended. Otherwise, if it does not match any of the flags I check against, I update the job to be RUNNING. I then give terminal control back to the shell. If bg was the command, I have a helper function -- bg_helper-- which also parses to get the jid, error checks, calls kill(), and updates the job to be RUNNING.

Finally, at any point where I call exit or reach EOF, I make sure to clean up the job list. 
