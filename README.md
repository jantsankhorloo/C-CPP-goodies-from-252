# Systerm Programming Projects at Purdue University

## lab2-src: FIZ/FLIZ
The FIZ is an interpreted LISPish scripting language. The parsing is done with LEX and YACC. FIZ does not suppert list operations. For that reason, FLIZ is a FIZ extension language where you can do list operations. This is an individual project.

## lab3-src: Implementing a Shell
The goal is to build a shell interpreter like csh, sh, bash, etc. This is an individual project.

## lab4-src: Threads
Implementation of Read and Write lock using pthread. This is an individual project.

## lab5-src: HTTP Web Server
It allows a HTTP client to connect and send GET request. This will accept custom ports, 1024-65536. This is an individual project.

## lab6-src: Processes
The program forks itself and kills other programs' processes, in order to have the most number of its duplicates remaning in the arena. Each arena restricts the number of processes it can have to N. This is a 2-person team project.
```
./arena N [my_program_name] [opponent_program_name] [...]... 
```