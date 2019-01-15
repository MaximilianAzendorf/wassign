# wsolve Tutorial

## Usage
Type `wsolve --help` to get information about command line usage of wsolve.

## Input format
Per default, the input is read from the standard input stream. You can also specify an input file with the appropriate command line flag. The input has to follow a certain format. Below you see an example input:

    (slot) Morning
    (slot) Afternoon
     
    (workshop) WSOLVE Usage: John Doe, 3-4
    (workshop) How to hold a Workshop: Jane Adams, 2-5
     
    (person) John Doe:       1 2
    (person) Jane Adams:     2 1
    (person) Richard Roe:    1 2
    (person) Joe Bloggs:     1 2
    (person) Thomas Roberts: 1 2
    (person) Ben Dover:      2 1 

There are three types of lines in an input file:

 - Slot descriptions: They begin with `(slot)`, followed by a name.
 - Workshop descriptions: They begin with `(workshop)` and have the following form: `(workshop) [workshop-name]: [workshop-tutor], [min-number-of-participants]-[max-number-of-participants]`.
 - Person descriptions: They begin with `(person)` and have the following form: `(person) [name]: [preference for workshop n]{N times}`, where N is the number of workshops. The preferences are given in the order in which the workshops appear in the input.

 
