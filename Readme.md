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
    
    (constraint) Yoga.Slot != Morning

There are four types of lines in an input file:

- Slot descriptions: They begin with `(slot)`, followed by a name.
- Event descriptions: They begin with `(event)` and have the following form: They begin with a name, followed by a colon and zero or more fixed participants (e.g. people which conduct the event). After that, each workshop has a participant range and (enclosed in square brackets) optional modifiers. Currently, there are two possible modifiers:
  - `[n parts]`: This event has multiple parts, meaning that it will occupy *n* consecutive slots. Internally, the workshop will be transformed into *n* workshops and an `EventSeries(part1, part2, ...)` constraint.
  - `[optional]`: This event is optional, meaning that it does not have to take place. wsolve is free to discard it and not schedule it at all.
- Person descriptions: They begin with `(person)` and have the following form: `(person) [name]: [preference for event n]{N times}`, where *N* is the number of events. The preferences are given in the order in which the events appear in the input.
- Additional constraints: You can specify additional constraints which the scheduling and/or assignment solution have to satisfy. More on that under section "Extra constraints"

## Extra constraints
You can specify extra conditions which the solution must satisfy. These extra conditions can be expressed in C#-Syntax. The following utility functions and (pseudo-)collections are exposed:

- `Participants`, `Events`, `Slots`: These are collections containing all participants, events and slots respectively.
- `Participant(name)`, `Event(name)`, `Slot(name)`: Gets the participant, event or slot with the given name. The given name does not have to be exact; `Event("W1")` will find an event with name `"W1"`, `"W1 How to hold an event"` and `"[W1] How to hold an event"`, in this order of precedence.
- For all participants, events and slots, there are auto-generated properties named after the first word in the name. For example, an event called `"W1 How to hold an event"` will be accessible through a property with the name `W1`, so instead of using `Workshop("W1").Name` you can write `W1.Name`. Note that on naming collisions some of these properties may be omitted; in this case, a warning will be printed.
- `EventSeries(workshop1, workshop2, ...)`: You can specify one or more workshops. This constraint will ensure that
  - All given workshops are scheduled to consecutive slots in the order given and
  - All workshops have the same participant list, so a participant is either in all of the given workshops or in none of them.
- Given a Participant `p` (e.g. `var p = Participant("John")` or `var p = John`):
  - `p.Events`: A collection containing all events this participant is assigned to.
- Given an event `w` (e.g. `var w = Event("W1")` or `var w = W1`):
  - `w.Participants`: A collection containing all participants that are assigned to this event.
  - `w.Conductors`: A collection cintaining all conductors of this event
  - `w.Slot`: Returns the slot this event is assigned to.
  - `w[n]`: Given an integer *n*, returns the *n*-th part of this workshop (with *n*=1 being the first part). Note that accessing a workshop *w* with multiple parts without an index always returns the first part, so a constraint `w.Slot != Morning` is the same as `w[1].Slot != Morning`.
- Given a slot `s` (e.g. `var s = Slot("Morning")` or `var s = Morning`):
  - `s.Events`: A collection containing all events that are assigned to this slot.
  
Note that collections like `Slot(...).Events` are not real collections, so you can not iterate over them or use Linq expressions. The following expressions are currently supported on these collections:
- Equalty: This can be expressed by the method `SequenceEqual(other)` or by the `==` operator.
- Contains: This can be expressed by the method `Contains(element)`.

Constraints can be negated by the `!` operator. Note that this is currently not supported for all types of constraints.

For implementation details, see [ExtraConditionsCodeEnvironment.txt](wsolve/resources/ExtraConditionsCodeEnvironment.txt), [ExtraConditionsBase.cs](wsolve/ExtraConditionsBase.cs) and [ExtraConditionsCompiler.cs](wsolve/ExtraConditionsCompiler.cs).

## The algorithm used

For information about the algorithms used, see [Algorithm.md](Algorithm.md).