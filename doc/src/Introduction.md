# Introduction to wassign

We will use the following scenario throughout this introduction in order to introduce all the features of wassign:

## A simple example

You are planning a convention where every participant has given preferences to different workshops that will be held at this convention. There are four events:

 - Workshop A: *How to become famous* (between 1 and 4 participants)
 - Workshop B: *Paleo cooking for beginners* (2--9 participants)
 - Workshop C: *Left-handed scissors: A critical review* (2--5 participants)
 - Workshop D: *Should you invest in bitcoin now?* (1--6 participants)

and 10 participants which gave the following preferences (between 0 and 10) to each of the workshops:

    Ethan Fanny Gavin Hanna Isaac July Kevin Lily Mark Norah
--- ----- ----- ----- ----- ----- ---- ----- ---- ---- -----
A   10    8     10    5     8     8    0     10   10   9
B   6     10    4     0     5     0    0     9    3    5
C   0     0     1     0     5     0    10    6    0    1
D   5     4     7     10    10    10   0     5    0    10
--- ----- ----- ----- ----- ----- ---- ----- ---- ---- -----

We now want to assign each of our 10 participants to one of the four events using wassign. To do this, we first have to translate our problem into terms that wassign can understand. We can use two core concepts of wassign to solve our example problem:

 - *choices* are things that can be chosen by choosers.
 - *choosers* are things that have defined a preference for every choice.

You can see that in our example, the workshops are the *choices* and our participants are the *choosers*. We now have to put our data into an input file that wassign understands. For our example, a basic input file would look like this:

```
+choice("How to become famous",                    bounds(1, 4));
+choice("Paleo cooking for beginners",             bounds(2, 9));
+choice("Left-handed scissors: A critical review", bounds(2, 5));
+choice("Should you invest in bitcoin now?",       bounds(1, 6));

+chooser("Ethan", [10, 6, 0, 5]);
+chooser("Fanny", [8, 10, 0, 4]);
+chooser("Gavin", [10, 4, 1, 7]);
+chooser("Hanna", [5, 0, 0, 10]);
+chooser("Isaac", [8, 5, 5, 10]);
+chooser("July",  [8, 0, 0, 10]);
+chooser("Kevin", [0, 0, 10, 0]);
+chooser("Lily",  [10, 9, 6, 5]);
+chooser("Mark",  [10, 3, 0, 0]);
+chooser("Norah", [9, 5, 1, 10]);
```

The details of the syntax required for these input files (and other neat features like reading things from CSV files so you don't have to type all these lines by yourself) can be found in the [manual](#using-wassign). If we put this input file through wassign with

```
wassign -i our-input-file.txt -o output-file
```

we promptly get back the following result as a CSV file (called `output-file.assignment.csv`):

```
"Choice", "Generated Slot"

"Ethan",  "Paleo cooking for beginners"
"Fanny",  "Paleo cooking for beginners"
"Gavin",  "How to become famous"
"Hanna",  "Should you invest in bitcoin now?"
"Isaac",  "Left-handed scissors: A critical review"
"July",   "Should you invest in bitcoin now?"
"Kevin",  "Left-handed scissors: A critical review"
"Lily",   "Paleo cooking for beginners"
"Mark",   "How to become famous"
"Norah",  "Should you invest in bitcoin now?"
```

Which translates to the following assignment:

- "How to become famous" is attended by Gavin and Mark,
- "Paleo cooking for beginners" is attended by Ethan, Fanny and Lily,
- "Left-handed scissors: A critical review" is attended by Isaac and Kevin and
- "Should you invest in bitcoin now?" is attended by Hanna, July and Norah.

You may note the "Generated Slot" column header in the output file. This is present because we did not specify any slots in our input (so wassign generated one for us); in the next section we will look at slots and what they can be used for.

## Getting more complicated with slots

Lets say that the time schedule of our convention (which is held in a single day) looks as follows:

-------------- ----------------------
10:00 -- 11:00 Welcome speech
11:00 -- 12:30 *Worshops I*
12:30 -- 14:00 Lunch break
14:00 -- 15:30 *Workshops II*
15:30 -- 17:00 After-convention party
-------------- ----------------------

As you can see, we actually have *two* timeslots where workshops will be held. But which workshop should go into which timeslot? We could now go on and assign our workshops to the timeslots by hand, but we can actually use wassign to do this for us based on the preferences the participants gave, so workshops are assigned to timeslots in such a way that the preferences of our participants can be satisfied as much as possible.

To do this, we use another core concept of wassign called *slots*. Slots are simply "buckets" where the choices will be distributed to. Then, every participant gets assigned to a choice *in every slot*. 

We just have to modify our input file slightly:

```
+slot("Workshops I");
+slot("Workshops II");

+choice("How to become famous",                    bounds(1, 4));
+choice("Paleo cooking for beginners",             bounds(2, 9));
+choice("Left-handed scissors: A critical review", bounds(2, 5));
+choice("Should you invest in bitcoin now?",       bounds(1, 6));

+chooser("Ethan", [10, 6, 0, 5]);
+chooser("Fanny", [8, 10, 0, 4]);

... (the rest of our "old" input file) ...
```
If we now input this into wassign with

```
wassign -i our-input-file.txt -o output-file -t 3s
```

it will give us the following *two* output files after 3 seconds (the `-t 3s` option we gave wassign this time tells the program to search exactly 3 seconds for the best possible solution):

A file called `output-file.scheduling.csv`

```
"Choice",                                   "Slot"

"How to become famous",                     "Workshops II"
"Paleo cooking for beginners",              "Workshops II"
"Left-handed scissors: A critical review",  "Workshops I"
"Should you invest in bitcoin now?",        "Workshops I"
```

describing the scheduling of the workshops into the slots and a file called `output-file.assignment.csv`

```
"Chooser", "Workshops I",                              "Workshops II"

"Ethan",   "Should you invest in bitcoin now?",        "Paleo cooking for beginners"
"Fanny",   "Should you invest in bitcoin now?",        "Paleo cooking for beginners"
"Gavin",   "Should you invest in bitcoin now?",        "How to become famous"
"Hanna",   "Should you invest in bitcoin now?",        "How to become famous"
"Isaac",   "Left-handed scissors: A critical review",  "Paleo cooking for beginners"
"July",    "Should you invest in bitcoin now?",        "How to become famous"
"Kevin",   "Left-handed scissors: A critical review",  "Paleo cooking for beginners"
"Lily",    "Left-handed scissors: A critical review",  "Paleo cooking for beginners"
"Mark",    "Left-handed scissors: A critical review",  "How to become famous"
"Norah",   "Should you invest in bitcoin now?",        "Paleo cooking for beginners"
```

describing the assignment of the participants into the workshops.

As you can see, we got exactly what we were looking for: wassign tells us which workshop should go into which time slot and at the same time gives uns the corresponding assignment for the participants.

## Specifying custom constraints

We now assume that Lily called us and told us that she can't attend the paleo cooking workshop because she is a vegetarian. We can use another feature of wassign called *constraints* to tell wassign that Lily must not be assigned to the paleo workshop under any circumstance. We do this by adding the following line to our input file:

```
+constraint( chooser("Lily").choices.contains_not(choice("Paleo")) );
```

Note that we do not have to specify the full name of the workshop; if there are no ambiguities, you can just use the first word of the name (in this case "Paleo").

wassign will now give us a different solution where Lily is not in the paleo workshop. Try it out!

## What else?

If you want to get an exhaustive overview over all features of wassign, you should consult the [manual](#using-wassign). Examples of things wassign can also do that were not covered in this short introduction:

 - *optional choices*: Choices marked with optional can be omitted completely if this allows wassign to create a better solution. Example: `+choice("XYZ", optional)`.
 - *multi-part choices*: Choices can have multiple parts (so they get scheduled into multiple consecutive slots at the same time). Example: `+choice("XYZ", parts(3))`.
 - advanced input files: The input files actually are scripts written in [chaiscript](http://chaiscript.com/), so you can go wild with typical programming constructs (conditionals, loops etc.) to build advanced logic into a single input file.
