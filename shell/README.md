# EC440: Shell 
Utilized the starter code from my shell parser:

The sample code provided for the Parser includes:
- [Makefile](Makefile):  The Makefile that builds your parser, tests, and runs your test programs as described [here](https://openosorg.github.io/openos/textbook/intro/tools-make.html#a-simple-example)
- [myshell_parser.h](myshell_parser.h): The header file that describes the structures and interfaces that clients of your parser needs to know (see [this](https://openosorg.github.io/openos/textbook/intro/tools-testing.html#testing)).
- [myshell_parser.c](myshell_parser.c): An initial stub implementation of the functions that you will need to fill in.
- [run_tests.sh](run_tests.sh): A shell script, described [here](https://openosorg.github.io/openos/textbook/intro/tools-shell.html#shell) for automating running all the tests

Two example test programs that exercise the interface of the parser, discussed [here](https://openosorg.github.io/openos/textbook/intro/tools-testing.html#testing), are:
- [test_simple_input.c](test_simple_input.c): Tests parsing the command 'ls'
- [test_simple_pipe.c](test_simple_pipe.c): Tests parsing the command 'ls | cat`
  <br>
  <br>
**Resources:**
- singals (sigchild) for Background handling: [oracle document](https://docs.oracle.com/cd/E19455-01/806-4750/signals-7/index.html)
- how to check if Ctrl + d: [where i got idea](https://stackoverflow.com/questions/19228645/fgets-and-dealing-with-ctrld-input) : [eof doc](https://www.tutorialspoint.com/c_standard_library/c_function_feof.htm)
- fork and piping documentation: [link](https://tldp.org/LDP/lpg/node11.html#:~:text=To%20create%20a%20simple%20pipe,be%20used%20for%20the%20pipeline.)
