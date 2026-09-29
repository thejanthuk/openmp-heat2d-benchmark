# Post-mortems

> **Template/planning file written by Claude (an AI assistant).** Entries below are Claude's unless marked otherwise.

**Post-mortem the results, not just the incidents.** "This number was wrong for three weeks and I
reported it" deserves the same treatment as a broken build — more, in computational science, because
a wrong number propagates into a report, a paper, and someone else's citation.

**Write one when:** a result I reported turned out wrong · a bug took more than two hours · a design
decision got reversed · a week slipped badly · anything I would rather my supervisor not discover
on his own.

## Template — `YYYY-MM-DD-short-slug.md`

```
# <what was wrong>

## Timeline
Written FIRST, before any interpretation. Commits, dates, what was believed when.
The universal failure mode is picking a cause and then fitting the timeline to it.

## What made this reasonable at the time
Assume competence. The question is never "who was careless" but "what made this the sensible
thing to do?" If the answer is "nothing, I was sloppy", this post-mortem has failed — it stopped
before reaching anything actionable.

## Trigger / Cause / Contributing factors
- Trigger:      what finally made it visible
- Cause:        what made it possible
- Contributing: everything that let it survive undetected
Only the contributing factors are fixable in advance, so that list matters more than the cause.
Branch the "why" rather than chaining it — real failures have several parents.

## Detection
How long was this wrong before anyone noticed, and what would have caught it sooner?
Usually the most actionable section in the document.

## What went right
Keep this section. A post-mortem that lists only failures teaches people to hide failures.

## Action items
Specific, owned, verifiable. "Be more careful" is not an action item.
"Assert omp_is_initial_device()==0 in every offload kernel and make the harness refuse to emit a
GPU row when it fails" is.
```

## Index

- `2026-09-22-tsan-openmp-false-positives.md` — Claude's toolkit claimed GCC TSan was clean on correct OpenMP code; it is not.
- `2026-09-29-scrub-verification-failures.md` — two checks during the public-release scrub that did
  not check what they claimed: a rule that missed a historical spelling, and a verification pattern
  that matched something deliberately kept.
