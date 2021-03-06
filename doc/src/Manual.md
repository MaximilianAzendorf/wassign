# Using wassign

The following section contains the complete documentation of all wassign features and how to use them.

## Input syntax

Input files are [chaiscript scripts](http://chaiscript.com/), so all typical programming constructs like variables (`var x = ...`), loops (`for(...)`), conditionals (`if(...)`) and others are available to construct the input. In addition, the following API is used to interact with wassign.

### Adding slots, choices choosers and constraints

+---+
| `slot(name)` |
+===+
| Creates a new slot with the given name or returns the slot with the given name if a slot with such name was already created. Note that if you create a new slot you still have to add it to the input with `add` or `+`. |
+---+ 

+---+
| `choice(name)`<br>`choice(name, args...)` |
+===+
| Creates a new choice with the given name and arguments (see [choice arguments](#choice-arguments) for more information) or (if just a name is given) returns the choice with the given name if a choice with such name was already created. Note that if you create a new choice you still have to add it to the input with `add` or `+`. |
+---+ 

+---+
| `chooser(name)`<br>`chooser(name, preferences)` |
+===+
| Creates a new chooser with the given name and preference list (e.g. `chooser("Karl", [100, 50, 0])`) or (if just a name is given) returns the chooser with the given name if a chooser with such name was already created. The preferences have to be in the same order in which the corresponding choices were added to the input. Note that if you create a new choice you still have to add it to the input with `add` or `+`. |
+---+ 

+---+
| `constraint(expression)` |
+===+
| Creates a new constraint from the given expression. For more information on how to construct constraint expressions, see the [respective section](#constraints) Note that if you create a new constraint you still have to add it to the input with `add` or `+`. |
+---+ 

+---+
| `add(arg)`<br>`+arg` (operator) |
+===+
| Adds the given argument `arg` (a newly created slot, choice, chooser or constraint) to the input. |
+---+ 

### Choice arguments

---------------- ---
`min(x)`         The choice must have at least `x` participants (e.g. `+choice("Foo", min(5))`). The default is 1.
`max(x)`         The choice must have at most `x` participants (e.g. `+choice("Foo", max(10)`). The default is 1.
`bounds(x, y)`   The choice must have at least `x` and at most `y` participants. This is the same as `min(x), max(y)`.
`parts(x)`       The choice has `x` parts. This means that the choice will be scheduled to `x` consecutive slots (in the order they were added to the input), and each chooser assigned to this choice will have this choice in the assignment at each of the slots.
`optional`       The choice is optional, this means that this choice may not be scheduled at all. If the choice is not scheduled there will also be no chooser that get assigned to this choice.
`optional_if(x)` The choice is optional if `x` is `true`.
---------------- ---

As an example, a choice named `Foo` that must have between 5 and 12 participants, is optional and has 3 parts would look like this in the input:

```
+choice("Foo", bounds(5, 12), parts(3), optional);
```

### Constraints

#### Constraint objects

Constraints are relations between two *constaint objects*. All slots, choices and choosers are *simple* constraint objects. In addition to them, the following derived constraint objects can be used to construct constraints:

+---+
| `SLOT.choices`
+===+
| A *list* constraint object describing all choices that are scheduled to the slot `SLOT`. |
+---+ 

+---+
| `SLOT.size`
+===+
| A *numerical* constraint object describing the number of choices that are scheduled to the slot `SLOT`. |
+---+ 

+---+
| `CHOICE.slot`<br>
| `CHOICE.slot(part)`
+===+
| A *simple* constraint object describing the the slot to which choice `CHOICE` is scheduled to. You can specify a part index (starting at 0) to refer to a specific part of the choice; by default, `part=0`. |
+---+ 

+---+
| `CHOICE.choosers`
+===+
| A *list* constraint object describing the the list of choosers that are assigned to the choice `CHOICE`. |
+---+ 

+---+
| `CHOOSER.choices`
+===+
| A *list* constraint object describing the the list of choices that the chooser `CHOOSER` is assigned to. |
+---+ 

#### Relational operators

+---+
| Relations between simple constraint objects<br><br>`left == right` (equality)<br>`left != right` (inequality)
+===+
| States that the simple constraint object `left` must be equal (or inequal) to the simple constraint object `right`. |
+---+ 

+---+
| Relations between numerical constraint objects<br><br>`left == right` (equality)<br>`left != right` (inequality)<br>`left > right` (greater-than)<br>`left >= right` (greater-or-equal-than)<br>`left < right` (less-than)<br>`left <= right` (less-or-equal-than) |
+===+
| States that the given relation must hold true between the numerical constraint object `left` and the numerical constraint object `right`. Note that `right` can also be a numerical constant. |
+---+ 

+---+
| Relations between list constraint objects<br><br>`left == right` (equality)<br>`left != right` (inequality)<br>`left.contains(right)` (contains-relation)<br>`left.contains_not(right)` (contains-not-relation) |
+===+
| States that the given relation must hold true between the list constraint object `left` and the list constraint object `right`, or (in the case of `contains` and `contains_not`) the *simple* constraint object `right`. |
+---+ 

#### Examples

 - `choice("X").slot != slot("Y")`: Choice "X" must not be in slot "Y".
 - `slot("Y").size <= 5`: Slot "Y" must have 5 or less choices scheduled to it.
 - `chooser("X").choices.contains_not(choice("Y"))`: Chooser "Y" must not be assigned to choice "Y".

### Reading from CSV files

+---+
| `read_csv(filename)`<br>`read_csv(filename, separator)` |
+===+
| Reads the given file as a [CSV file](https://en.wikipedia.org/wiki/Comma-separated_values), optionally specifying a separator (the default is `,`). |
+---+ 

+---+
| `CSVFILE.row(n)`<br>`CSVFILE[n]` |
+===+
| Returns the row with number `n` (numbering starting at 0) of the `CSVFILE` (returned by `read_csv`). Rows are just plain lists of the row values, so individual values of a row can be accessed with the `[]` operator.|
+---+ 

+---+
| `CSVFILE.rows` |
+===+
| Returns all rows of the `CSVFILE` (returned by `read_csv`). |
+---+

#### Example

Given is the following CSV file called `workshops.csv`:

```
"Choice", "minimum", "maxiumum", "optional?"
"A",      5,         10,         "no"
"B",      5,         20,         "no"
"C",      10,        30,         "yes"
```

We can then use this file to generate choices for our input as follows:

```
var file = read_csv("workshops.csv");

for(row : file.rows.slice(1, end))
{
    +choice(row[0], bounds(row[1], row[2]), optional_if(row[3] == "yes"));
    // or add(choice(...))
}
```

Note that we have to skip the first row (with `.slice(1, end)`) because it contains the column headers.

### Utility functions

+---+
| `LIST.slice(x, y)` |
+===+
| Returns a list that only contains the element of the list `LIST` with indices between `x` (inclusive) and `y` (inclusive). Note that index numbering starts at 0. You can give `end` as the value for `y` as a replacement for `LIST.length - 1`. |
+---+

+---+
| `range(x, y)` |
+===+
| Returns a list that contains all integers between `x` (inclusive) and `y` (inclusive) in order. |
+---+

## Program options

----------------------------------- ---
`-h`, `--help`                      Show a short summary of all program options.
`--version`                         Show the current version of wassign.
`-i [file]`, `--input [file]`       Read the input from the specified file(s). If this option is present more than once, all files will be read in the order they were given.
`-o [prefix]`, `--output [prefix]`  Write the output to files starting with the given prefix. The files generated will be named `[prefix].assignment.csv` and `[prefix].scheduling.csv`.
`-v [n]`, `--verbosity [n]`         A number `n` between 0 and 3 indicating how much status information should be output.
`-a`, `--any`                       If this option is given, wassign will not optimize any solution and will just return the first solution it finds.
`-p [exp]`, `--pref-exp [exp]`      Sets the preference exponent to the given value. See the [respective section](#preference-exponent) for more information.
`-t [time]`, `--timeout [time]`     Sets the optimization timeout. The syntax for this argument is described under the [respective section](#time-format).
`--cs-timeout [time]`               Sets the timeout for attempting to satisfy critical sets of a certain preference level. Higher values may lead to better initial solutions, but it may take longer to find an initial solution in the first place. The syntax for this argument is described under the [respective section](#time-format).
`--no-cs`                           If this option is given, no critical set analysis is performed.
`-j [n]`, `--threads [n]`           Specifies the maximum number of computation threads. By default, wassign will use as many threads as there are logical CPU cores on the system.
`-n [n]`, `--max-neighbors [n]`     Specifies the maximum number of neighbor schedulings that will be explored per hill climbing iteration.
`-g`, `--greedy`                    If this option is given, wassign will not use the worst-preference scoring as a primary score and will instead just use sum-based scoring instead.
----------------------------------- ---

### Preference exponent 

The preference exponent is a parameter of wassign that directly affects which solutions are considered "fairer" than others.

As a rule of thumb: The higher the preference exponent, the more "even" the assignment will be, in the sense that e.g. wassign will prefer to give many people their second choice instead of giving few their first and many their third.

### Time format

Program options like `--timeout` require a time string that has the following format: A time string consists of one or more components of the form `[number][unit]`, concatenated without spaces. Examples:

 - `10s` is a timespan of ten seconds.
 - `5h` is a timespan of five hours.
 - `1d30m` is a timespan of one day and 30 minutes.
 - `2w3d5h7m11s` is a timespan of 2 weeks, 3 days, 5 hours, 7 minutes and 11 seconds.
