------------- Readme -------------
SEE !!!Note before compiling.

Group 7 members:
	Jacob Nemeth, Josh Lowe, Elizabeth Fezzie
Division of Labor (Final):
	Part 1:
		Josh Lowe, Jacob Nemeth, Elizabeth Fezzie
	Part 2:
		Josh Lowe, Jacob Nemeth, Elizabeth Fezzie
	Part 3:
		Josh Lowe, Jacob Nemeth, Elizabeth Fezzie
	Part 4:
		Josh Lowe, Jacob Nemeth
	Part 5:
		Josh Lowe, Jacob Nemeth
	Part 6:
		Josh Lowe, Jacob Nemeth
	Part 7:
		Josh Lowe, Jacob Nemeth, Elizabeth Fezzie
	Part 8:
		Josh Lowe, Jacob Nemeth
	Part 9:
		Josh Lowe
	Part 10:
		Josh Lowe, Jacob Nemeth, Elizabeth Fezzie

List of files:
	parser.c - In this file, we implemented all 10 parts of project 1 inside of this c file. Using the makefile, we are able to easily run this file with two simple commands.
	Makefile - The makefile allows us to run our shell easily and effectively.
	
How to compile executables (these are the two commands we used to run our makefile): 
	1. Make
		!!!Note: If there's an error while making, you may need to go in and change 8 spaces to 2 tabs on lines 8 and/or 11. We've had the same
		issue when downloading code from this repository, and weren't able to find the issue. The error is of the form:
		"Makefile:8: *** missing separator (did you mean TAB instead of 8 spaces?).  Stop."
	2. shell

Known bugs:
	$PATH Search:
		We didn't get to checking if a command does not contain a slash, though functionality works as expected.
	Piping:
		When piping a single command, the contents are correctly outputted. However, they are printed twice.
		Piping with multiple pipes does not work. We spliced the data properly, but could not figure out how the two pipes become connected. When
		trying to fix this bug, we tried to splice the data into new token list where tokenlist1 would read up to the first '|' character and so
		on. However, we believe that the main issue we were facing was with connecting to pipes with multiple file descriptors.
	Background Processing:
		We got background processes to spawn, as you can test with "sleep 5" followed by "sleep 5 &". The main issue here was that we tried
		to implement a queue and the implementation itself worked. We then had a global queue where we wanted to store background process
		information. When attempting to enqueue in a background process, we ran into issues, preventing us from putting the correct process
		information into the queue.
	Built in commands: 
		Jobs - Jobs was not implemented as we have some functionality issues with background processing described above.
		Exit - When we use a built-in command, we cannot exit properly, as you need to type in exit twice.
	Background processing:
		
		
Special considerations:
	N/A
Extra Credit:
	I/O redirection and Piping in the same commands (output is not right, but it does support it). The output is what you would expect from
	a pipe, except a command prompt prints at the bottom of the output file.
