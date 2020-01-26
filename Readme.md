# wsolve Usage Guide

## Usage
See `wsolve --help` to get basic information about command line usage of wsolve.

## Input format
Per default, the input is read from the standard input stream. You can also specify one or more input files (which will be read in the order given) with the appropriate command line flag. The input has to follow a certain format. Below you see an example input:

    (slot) Morning
    (slot) Afternoon
     
    (event) Introduction to wsolve: John Doe, 3-4 [2 parts]
    (event) Yoga: 2-6 [optional]
    (event) How to hold an event: Jane Adams+John Doe, 2-5
     
    (person) John Doe:       1 3 2
    (person) Jane Adams:     2 1 3
    (person) Richard Roe:    3 1 2
    (person) Joe Bloggs:     3 1 2
    (person) Thomas Roberts: 1 3 2
    (person) Ben Dover:      2 1 3 
    
    (constraint) slot of [Yoga] is not [Morning]

There are four types of lines in an input file:

- Slot descriptions: They begin with `(slot)`, followed by a name.
- Event descriptions: They begin with `(event)` and have the following form: They begin with a name, followed by a colon and zero or more fixed participants (e.g. people which conduct the event). After that, each workshop has a participant range and (enclosed in square brackets) optional modifiers. Currently, there are two possible modifiers:
  - `[n parts]`: This event has multiple parts, meaning that it will occupy *n* consecutive slots. Internally, the workshop will be transformed into *n* workshops and appropriate constraints.
  - `[optional]`: This event is optional, meaning that it does not have to take place. wsolve is free to discard it and not schedule it at all.
- Person descriptions: They begin with `(person)` and have the following form: `(person) [name]: [preference for event n]{N times}`, where *N* is the number of events. The preferences are given in the order in which the events appear in the input.
- Additional constraints: You can specify additional constraints which the scheduling and/or assignment solution have to satisfy. More on that under section "Extra constraints"

## Extra constraints

You can specify extra conditions which the solution must satisfy.

## The algorithm used

For information about the algorithms used, see [Algorithm.md](Algorithm.md).