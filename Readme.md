# wsolve Usage Guide

## Input format
Per default, the input is read from the standard input stream. You can also specify an input file with the appropriate command line flag. The input has to follow a certain format. Below you see an example input:

    (slot) Morning
    (slot) Afternoon
     
    (workshop) WSOLVE Usage: John Doe, 3-4
    (workshop) How to hold a Workshop: Jane Adams+John Doe, 2-5
     
    (person) John Doe:       1 2
    (person) Jane Adams:     2 1
    (person) Richard Roe:    1 2
    (person) Joe Bloggs:     1 2
    (person) Thomas Roberts: 1 2
    (person) Ben Dover:      2 1 

There are three types of lines in an input file:

- Slot descriptions: They begin with `(slot)`, followed by a name.
- Workshop descriptions: They begin with `(workshop)` and have the following form: `(workshop) [workshop-name]: [workshop-tutor(s)], [min-number-of-participants]-[max-number-of-participants]`.
- Person descriptions: They begin with `(person)` and have the following form: `(person) [name]: [preference for workshop n]{N times}`, where N is the number of workshops. The preferences are given in the order in which the workshops appear in the input.

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
- `Participants`, `Workshops`, `Slots`: These are collections containing all participants, workshops and slots respectively.
- `Participant(name)`, `Workshop(name)`, `Slot(name)`: Gets the participant, workshop or slot with the given name. The given does not have to be exact; `Workshop("W1")` will find a Workshop with name `"W1"`, `"W1 How to hold a Workshop"` and `"[W1] How to hold a workshop"`, in this order of precedence.
- Given a Participant `p` (e.g. `var p = Participant("John")`):
  - `p.Name`: The name of the participant.
  - `p.Workshops`: A collection containing all workshops this participant is assigned to.
  - `p.WorkshopAt(slot)`: Returns the workshop this participant is assigned to in slot `slot`.
- Given a Workshop `w` (e.g. `var w = Workshop("W1")`):
  - `w.Name`: The name of the workshop.
  - `w.MinParticipants`: The minimum number of participants of this workshop.
  - `w.MaxParticipants`: The maximum number of participants of this workshop.
  - `w.Participants`: A collection containing all participants that are assigned to this workshop.
  - `w.Conductors`: A collection cintaining all conductors of this workshop.
  - `w.Slot`: Returns the slot this workshop is assigned to.
- Given a slot `s` (e.g. `var s = Slot("Morning")`):
  - `s.Name`: The name of the slot.
  - `s.Workshops`: A collection containing all workshops that are assigned to this slot.

All collections are of type `IEnumerable<T>`.

Apart from this API, the namespaces `System`, `System.Text`, `System.Linq` and `System.Collections.Generic` are available. Also, the class `System.Math` is statically imported (so you can use Functions like `Abs(...)` directly instead of `Math.Abs(...)`).

For implementation details, see [wsolve/resources/ExtraConditionsCodeEnvironment.txt], [wsolve/CustomFilter.cs] and [wsolve/ExtraConditionsCompiler.cs].

#### Examples:

##### Example 1
Workshops W1 and W2 must be in the same slot:

    AddCondition(Workshop["W1"].Slot == Workshop["W2"].Slot);

##### Example 2
Workshop W1 must have less participants than Workshop W2:

    AddCondition(Workshop["W1"].Participants.Count() < Workshop["W2"].Participants.Count());

##### Example 3
All Workshops in Slot "Morning" must have an even number of participants IF they are not full and Participant "John" must be in Workshop W3:

    foreach(Workshop w in Slot("Morning").Workshops)
    {
        if(w.Participants.Count() != w.MaxParticipants)
            AddCondition(w.Participants.Count() % 2 == 0);
    }

##### Example 4
Example 3 can also be accomplished with the usage of Linq's `Where`:

    foreach(Workshop w in Slot("Morning").Workshops.Where(w => w.Participants.Count() != w.MaxParticipants))
    {
        AddCondition(w.Participants.Count() % 2 == 0);
    }

##### Example 5
Participant John must be in Workshop W3:

    AddCondition(Participant("John").Workshops.Contains("W3"));

##### Example 6
Note that when you specify conditions directly with `--conditions=[condition]`, you have to omit the `AddCondition(...)`. The first example would then be:

    wsolve ... --conditions='Workshop["W1"].Slot == Workshop["W2"].Slot' ...