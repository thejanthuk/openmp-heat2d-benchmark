# Authorship

This is Krit Ponjanthuk's placement project at the host university. While learning, Krit worked with Claude (an AI
assistant from Anthropic) as a tutor and reviewer. This file says who wrote what, so the repository
can be read honestly as evidence of Krit's own work.

| Path | Written by |
|---|---|
| `include/heat/*.hpp`, `tests/*.cpp`, `src/` | **Krit.** Claude reviewed it and suggested changes; Krit wrote the code. |
| `NOTES.md` | **Krit**, except one entry explicitly marked *"DRAFTED BY CLAUDE"*. |
| `CMakeLists.txt`, `.gitignore` | Claude (build infrastructure), at Krit's request. |
| `CLAUDE.md` | Claude — instructions for the AI assistant. |
| `docs/design-decisions.md` | Decisions reached by Krit in discussion with Claude; entries drafted by Claude. |
| `docs/sessions/*.md` | Drafted by Claude from the session conversations. |
| `docs/00-gpu-forensics.md` and the other teaching docs in `docs/` | Claude, with measurements run on Krit's machine. |
| `docs/questions.md`, `docs/next-3-quests.md`, `docs/postmortems/README.md` | Claude (templates and planning). |
| `docs/build-and-run.md` | Claude (reference card). |
| `docs/audit-2026-09-22.md` | Claude, auditing Claude's own claims. |
| `docs/postmortems/2026-09-22-tsan-openmp-false-positives.md` | Claude, about Claude's own error. |
| `docs/reading-order.md` | Claude (reading plan for the two reference books). |
| `docs/book-recommendations.md` | Claude (researched book list for the career goal). |

Every Claude-written file in `docs/` also says so at the top.

**Not in this repository.** The project spec (`docs/private/project-spec.pdf`) and the supervisor's email
reply are his material, not Krit's, so they are held locally and gitignored rather than published
here. Section 6 of the spec is summarised in `CLAUDE.md`; the four-version structure and the
`collapse(2)` and `target data` requirements come from it.
