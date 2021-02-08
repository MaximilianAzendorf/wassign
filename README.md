wassign is a tool to solve an extension of the [stable marriage problem](https://en.wikipedia.org/wiki/Stable_marriage_problem).

### The problem

Given are three sets: slots, choices and persons. Each person has a specified preferences towards all choices (e.g. 100 meaning "i really want to attend this choice" and 0 meaning "i do not want to attend this choice").

We now want to match

1. events to slots, such that each event is assigned to exactly one slot
2. persons to events, such that each person is in exactly one choice *per slot*.

while respecting the preferences given by the persons "as much as possible". Additionally, each choice has a minimum and a maximum number of choosers.

### The solution

The stated problem can not be computed efficiently. wassign uses a combination of hill climbing and MIP to quickly find good solutions. The details of the used algorithms are described in the [wiki](https://github.com/MaximilianAzendorf/wassign/wiki).

### How do I build/use wassign?

See the [wiki](https://github.com/MaximilianAzendorf/wassign/wiki) for more information.
