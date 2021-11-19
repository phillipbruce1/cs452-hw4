# Homework 4 - It's *that* kind of Thursday

CS 452 - Prof. Buffenbarger

Author: Phillip Bruce

## Compilation and Execution

To compile:
```bash
$ make
$ make install
```

To run:
```bash
$ make try
```
The input the text you want scanned, hit "enter", then enter another line to make the program run. I do not know why it requires that second line, but it does.

## Purpose

The purpose of this program is to implement a simple scanner driver to be installed on a Linux machine. It takes a line of input from the user, then outputs tokens from said input based on specified separator characters.

## Challenges

To correct my above wording, this program *theoretically* implements a simple scanner driver. In reality, it does not...

It took me up until the day before the new due date of this project to understand what this assignment is and what was being asked of me. Once I understood however, I set to work.

I was unable to accomplish the actual token reading portion of this assignment. Unfortunately, the project currently only accepts input which is 7 characters or less and without spaces. I have no clue why the 7 characters as there is nowhere in the code I specified that, but it is what it is.

## Valgrind

The program seg faults, so valgrind did not give me full output...