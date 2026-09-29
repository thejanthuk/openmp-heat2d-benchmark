# Two verification failures while scrubbing the repository for public release

> **Written by Claude (an AI assistant)**, about Claude's own errors. Third person throughout.

Neither error reached the published repository. Both are recorded because they are the same *kind* of
failure the 2026-09-22 audit was written about — **a check that did not check what it claimed** — and
because the second one stopped work on a false alarm.

## Timeline

- **2026-09-29, earlier** — the repository was made public. Two history rewrites: one removing the
  supervisor's spec and emailed reply, one correcting the author email after the account it named
  turned out to be a different account from the one hosting the repository, rather than a rename.
- **Then** — Krit asked for every file to be read and personal details removed. An inventory over all
  tracked files and all commits produced five categories; he chose the scope, keeping his own name.
- Claude wrote a replacement script and **trialled it on a throwaway clone first.** That caught four
  broken sentences before any real history was touched.
- **Pass 1** of the content rewrite ran over all 22 commits. The post-rewrite check found **one**
  remaining hit: an intermediate commit still held a remote URL naming the old account.
- **Pass 2** ran with a regex fallback added. The scoped check then returned zero.
- **A deeper sweep over every blob in the object database reported 7 blobs still containing
  identifiers.** Claude raised this as possibly-unclean, stopped, and told Krit not to push.
- Krit ran the check. All 7 were versions of `AUTHORSHIP.md` matching on **his own surname**, which he
  had explicitly chosen to keep. The repository had been clean since pass 2.

## What made each reasonable at the time

**Error 1 — literal rules that missed a historical spelling.** The rules were written as whole
phrases *on purpose*. A trial had already demonstrated that blind substitution produces text like
"the old an earlier local guide guide". Precision was the correct instinct; the defect was not
extending it to the fact that one line had existed in more than one form.

**Error 2 — a verification pattern that matched something deliberately kept.** The pattern was
carried over unchanged from the *inventory* phase, where matching the surname was exactly right —
Claude needed to know where the name appeared before Krit could decide whether to keep it. The same
pattern was then reused for *verification*, where its job is the opposite.

## Cause

- **A string can have several spellings across history, and a rule written against the working tree
  sees only the newest.** The missed line was one Claude had itself edited earlier the same session,
  so its older form existed for only a few commits.
- **An inventory pattern and a verification pattern have different jobs.** An inventory should
  over-match: false positives cost a glance. A verification must match only what was meant to be
  removed, or "zero hits" stops meaning anything and a non-zero count raises a false alarm.

## Contributing factors

- **The trial cloned the tip, not the history.** A trial that only exercises the current files cannot
  reveal a historical-spelling miss, which is precisely the class of bug a history rewrite has.
- **Nothing in the tooling distinguished "remove this" from "keep this".** Both lists lived in
  Claude's head; only the removals were written down, in the script.
- The identifiers being removed and the identifier being kept were adjacent in the same sentence of
  the same file, so a single grep hit both.

## Detection

The two were caught very differently, and the difference is the lesson.

| | Caught by | Cost |
|---|---|---|
| Historical spelling | Claude's own post-rewrite scoped grep | one extra rewrite pass |
| False positive | Krit running a command and pasting the output | a round trip, and an unnecessary "do not push" |

The first is what a verification step is *for*. The second is a verification step producing noise and
consuming someone else's attention — the worse outcome of the two, even though nothing was wrong.

## What went right

- **Trialling the script on a throwaway clone before touching real history.** This caught four
  grammar failures, including gendered pronouns for a person whose pronouns are not stated.
- **A backup before every destructive step.** Three rewrites, two blocked commands and two stashes,
  and Krit's uncommitted work came back byte-identical each time, checked with `md5sum` and `diff`.
- **Final verification was done against a clone of the public URL**, not the local repository — the
  only check that actually describes what readers receive. It also confirmed the public clone builds
  and passes its tests, which would have caught an over-broad ignore rule.

## Action items

1. **Trial a history-rewriting script against historical versions, not just the tip.** Cloning the
   repo and running the script over `git rev-list` output would have found the missed line before the
   first pass, not after it.
2. **Done** — the remove-list and the keep-list are now one file, `docs/private/scrub-denylist.txt`,
   with the check that reads it written at the top. It lives in the gitignored directory because a
   public list of forbidden strings would publish the strings it forbids. Every check reads the file;
   none retypes the patterns.
3. **Prefer patterns to sentences when rewriting history.** The regex fallback added in pass 2 —
   matching any `git@github.com:…` URL rather than one spelling of one line — is the form the rules
   should have taken from the start.
4. **Verify a published repository by cloning its public URL**, and build it from that clone.
5. **Before raising an alarm that stops work, re-run the check with the narrowest pattern that could
   confirm it.** Claude had the information needed to resolve the 7-blob question without asking.

## The connection worth keeping

The 2026-09-22 audit concluded that this project's worst failures were **measurement failures, not
knowledge failures** — numbers compared across different accounting, or claims of "verified" that
rested on a check which could not have detected the problem. Both errors here are the same shape in a
different domain: the script's rules and the verification's pattern each looked authoritative and each
answered a slightly different question than the one being asked.

**A verification is a claim too.** It needs its own scope stated, exactly like a measurement needs its
byte accounting.

## Postscript — the same error, once more, while writing this file

The first draft of this post-mortem **named the removed account in its timeline**, reintroducing into a
tracked file exactly the identifier the scrub had removed. The check Claude ran against the new file
omitted that pattern from its list, so it reported "clean".

Three rewrites, an explicit inventory and a written action item were not enough to stop it, because
the same two defects recurred: the remove-list lived in Claude's head rather than on disk, and the
verification pattern was retyped from memory instead of being the one canonical list.

It was caught on a second look, before the file was committed, and the list is now on disk at
`docs/private/scrub-denylist.txt` with its keep-list beside it. **A list that is retyped each time is
not a list; it is a recollection.**
