**WASM branch:** *This is the web assembly branch. Run build.sh to build a web assembly binary of wsolve. Needs clang.*

wsolve is a tool to solve an extension of the [stable marriage problem](https://en.wikipedia.org/wiki/Stable_marriage_problem).

### The problem

Given are three sets: timeslots, events and persons. Each person has a specified preferences towards all events (e.g. 100 meaning "i really want to attend this event" and 0 meaning "i do not want to attend this event").

We now want to match

1. events to timeslots, such that each event is assigned to exactly one timeslot
2. persons to events, such that each person is in exactly one workshop *per slot*.

while respecting the preferences given by the persons "as much as possible". Additionally, each workshop has a minimum and a maximum number of participants.

### The solution

The stated problem can not be computed efficiently. wsolve uses a combination of hill climbing and MIP to quickly find good solutions. The details of the used algorithms are described in the [wiki](https://github.com/MaximilianAzendorf/wsolve/wiki).

### How do I build/use wsolve?

See the [wiki](https://github.com/MaximilianAzendorf/wsolve/wiki) for more information.
