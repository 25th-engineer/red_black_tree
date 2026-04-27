---
name: git-commit-no-exe
description: Review, stage, and commit repository changes while excluding .exe files from staging. Use when the user asks to add, stage, or commit files and expects a quick code review before commit.
---
# Git Commit Excluding .exe

## Purpose
Create commits safely while preventing `.exe` files from being committed by default.

## Workflow
1. Inspect repository state before staging:
   - `git status --short --branch`
   - `git diff --staged; git diff`
   - `git log --oneline -n 10` (if history exists)
2. Review files before staging:
   - For tracked files: `git diff -- <file>`
   - For untracked files: `git add -N -- <file>` then `git diff -- <file>`
   - Check for correctness issues, regressions, risky edge cases, and missing test updates.
   - Report findings before committing; fix blockers first.
3. Stage intended changes:
   - `git add -A`
4. Remove executables from staging:
   - `git restore --staged -- "*.exe" "**/*.exe"`
   - If `git restore --staged` is unavailable, use `git reset HEAD -- "*.exe" "**/*.exe"`
5. Verify staged files:
   - `git diff --cached --name-only`
   - Confirm no path ends with `.exe`
6. Commit only after user request:
   - Use a concise message focused on intent.
   - Keep `.exe` files untracked or unstaged unless the user explicitly asks to include them.

## Safety Rules
- Never use destructive git operations unless explicitly requested.
- Do not commit secrets or credential files.
- If only `.exe` files are staged, stop and ask for guidance instead of committing.
