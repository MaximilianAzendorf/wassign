# Using wassign

tbd

## Input syntax

## Program options

----------------------------------- ---
`-h`, `--help`                      Show a short summary of all program options.
`--version`                         Show the current version of wassign.
`-i [file]`, `--input [file]`       Read the input from the specified file(s). If this option is present more than once, all files will be read in the order they were given.
`-o [prefix]`, `--output [prefix]`  Write the output to files starting with the given prefix. The files generated will be named `[prefix].assignment.csv` and `[prefix].scheduling.csv`.
`-v [n]`, `--verbosity [n]`         A number `n` between 0 and 3 indicating how much status information should be given.
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

### Time format