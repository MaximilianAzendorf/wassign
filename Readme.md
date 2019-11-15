# wsolve Usage Guide

## Input format
Per default, the input is read from the standard input stream. You can also specify an input file with the appropriate command line flag. The input has to follow a certain format. Below you see an example input:

    (slot) Morning
    (slot) Afternoon
     
    (event) WSOLVE Usage: John Doe, 3-4
    (event) How to hold an event: Jane Adams+John Doe, 2-5
     
    (person) John Doe:       1 2
    (person) Jane Adams:     2 1
    (person) Richard Roe:    1 2
    (person) Joe Bloggs:     1 2
    (person) Thomas Roberts: 1 2
    (person) Ben Dover:      2 1 

There are three types of lines in an input file:

- Slot descriptions: They begin with `(slot)`, followed by a name.
- Event descriptions: They begin with `(event)` and have the following form: `(event) [event-name]: [event-tutor(s)], [min-number-of-participants]-[max-number-of-participants]`.
- Person descriptions: They begin with `(person)` and have the following form: `(person) [name]: [preference for event n]{N times}`, where N is the number of events. The preferences are given in the order in which the events appear in the input.

## Usage
See `wsolve --help` to get basic information about command line usage of wsolve.

### Function triplets
The options `--mutation`, `--crossover` and `--population` can not only take numbers (e.g. `--mutation=0.4`), but also so-called function triplets. these are of the form `[from]-[to]^[exponent]` and describe a function `f(x)` that interpolates from `[from]` to `[to]` using a polynomial function. The formula used for this is

    f(x) = ([to] - [from]) * (x^[exp]) + [from],

where `x` is a parameter between zero and one. Given this formula, the parameter `x` is 0 at the beginning of the calculations and gradually ascends to 1, which is reached when the final phase starts. The final phase starts (per default) at the last 20% of the calculation time (e.g. for a timeout of 10 minutes, the final phase starts at the 8 minute mark).

Using this feature, you can gradually change the mutation and crossover chances as well as the population size. For example, the population size has the default value `5000-40^1.8`, which means that the population size starts at 5000 and gradually gets lower until, at the start of the final phase, the population size is 40.

### Selection algorithms
wsolve currently supports two different selection schemes:

- Elite selection (option `--selection=elite`): The fittest solutions are chosen for crossover in strict ordering of fitness.
- Tournament selection (option `--selection=tournament([size])`): Solutions are chosen based on a tournament system. This mode has a parameter `[size]` which controls the behaviour of this selection algorithm. For more information, see https://en.wikipedia.org/wiki/Tournament_selection.

### Extra conditions
You can specify extra conditions which the solution must satisfy. These extra conditions can be expressed in C#-Syntax. The following utility functions and collections are exposed:

- `AddCondition(condition)`: Adds a boolean condition that must be satisfied by all solutions.
- `Participants`, `Events`, `Slots`: These are collections containing all participants, events and slots respectively.
- `Participant(name)`, `Event(name)`, `Slot(name)`: Gets the participant, event or slot with the given name. The given does not have to be exact; `Event("W1")` will find an event with name `"W1"`, `"W1 How to hold an event"` and `"[W1] How to hold an event"`, in this order of precedence.
- For all participants, events and slots, there are auto-generated properties named after the first word in the name. For example, an event called `"W1 How to hold an event"` will be accessible through a property with the name `W1`, so instead of using `Workshop("W1").Name` you can write `W1.Name`. Note that on naming collisions some of these properties may be omitted and a warning will be shown. 
- Given a Participant `p` (e.g. `var p = Participant("John")` or `var p = John`):
  - `p.Name`: The name of the participant.
  - `p.Events`: A collection containing all events this participant is assigned to.
  - `p.EventAt(slot)`: Returns the event this participant is assigned to in slot `slot`.
- Given an event `w` (e.g. `var w = Event("W1")` or `var w = W1`):
  - `w.Name`: The name of the event.
  - `w.MinParticipants`: The minimum number of participants of this event.
  - `w.MaxParticipants`: The maximum number of participants of this event.
  - `w.Participants`: A collection containing all participants that are assigned to this event.
  - `w.Conductors`: A collection cintaining all conductors of this event.
  - `w.Slot`: Returns the slot this event is assigned to.
- Given a slot `s` (e.g. `var s = Slot("Morning")` or `var s = Morning`):
  - `s.Name`: The name of the slot.
  - `s.Events`: A collection containing all events that are assigned to this slot.

All collections are of type `IReadOnlyCollection<T>`.

Apart from this API, the namespaces `System`, `System.Text`, `System.Linq` and `System.Collections.Generic` are available. Also, the class `System.Math` is statically imported (so you can use Functions like `Abs(...)` directly instead of `Math.Abs(...)`).

For implementation details, see [ExtraConditionsCodeEnvironment.txt](wsolve/Resources/ExtraConditionsCodeEnvironment.txt), [ExtraConditionsBase.cs](wsolve/ExtraConditionsBase.cs) and [ExtraConditionsCompiler.cs](wsolve/ExtraConditionsCompiler.cs).

#### Examples:

##### Example 1
Events W1 and W2 must be in the same slot:

    AddCondition(W1.Slot == W2.Slot);

##### Example 2
Event W1 must have less participants than event W2:

    AddCondition(W1.Participants.Count < W2.Participants.Count);

##### Example 3
All events in slot "Morning" must have an even number of participants IF they are not full:

    foreach(Event w in Morning.Events)
    {
        if(w.Participants.Count != w.MaxParticipants)
            AddCondition(w.Participants.Count % 2 == 0);
    }

##### Example 4
Example 3 can also be accomplished with the usage of Linq's `Where`:

    foreach(Event w in Morning.Events.Where(w => w.Participants.Count != w.MaxParticipants))
    {
        AddCondition(w.Participants.Count % 2 == 0);
    }

Or, as a one-liner:

    AddCondition(Morning.Events.Where(w => w.Participants.Count != w.MaxParticipants).All(w => w.Participants.Count % 2 == 0))

##### Example 5
Participant John must be in event W3:

    AddCondition(John.Events.Contains(W3));

##### Example 6
Note that when you specify conditions directly with `--conditions=[condition]`, you have to omit the `AddCondition(...)`. The first example would then be:

    wsolve ... --conditions="W3.Slot == W2.Slot" ...
