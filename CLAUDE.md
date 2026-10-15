# SFML_Clicker

## Context

Idle clicker game built solo in C++ with SFML, object-oriented, for control over performance/game feel/loading times. Inspired by the structure of "Taps of Eradine" but with no erotic content — familiars (like Pokémon/Digimon) replace that game's Nymphs, evolve into stronger forms, deal damage, and unlock new tools/playstyles for the player.

## Architecture philosophy

Learning SOLID principles, but applying them pragmatically — explain when a principle is actually worth applying here vs. when KISS is the better call. Don't apply patterns "because best practice" without reasoning tied to this specific codebase.

## How to give solutions

- Never modify project files directly (no file edits applied by the AI) without explicit permission for that specific change. Default: present the complete, ready-to-use file content in chat for review, and only apply it after I explicitly confirm (e.g. "go ahead", "apply it").
- Never give manual edit instructions ("change line X to Y") — I have ADHD and this causes copy/paste errors and missed spots. Always give complete, ready-to-use files.
- Use plain/self-taught terminology, not CS jargon (e.g. avoid "lambda" without explaining it in my terms) — I'm self-taught and unfamiliar names slow me down and create confusion.
- Briefly explain *why* something was implemented a certain way, unless I ask for more depth.

## Code quality bar

I'm autistic and want things done right the first time, not fast-then-refactored. When implementing a feature, design it to accommodate reasonably foreseeable future additions rather than needing a rewrite every time something new is added. Flag trade-offs if "extensible now" vs. "simple now" is a real tension.
