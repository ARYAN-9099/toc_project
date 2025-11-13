# Turing Machine: Smallest Palindrome With Given Prefix

This project simulates a single-tape Turing Machine (TM) that, given a lowercase string `s` (a..z), appends the minimum number of characters to the right of `s` to form the shortest palindrome that starts with `s`.

## Examples
- `aba` → already a palindrome → `aba`
- `aab` → `aabaa`
- `abac` → `abacaba`
- `abcd` → `abcdcba`

## Overview
- Input: a lowercase string (no spaces), e.g., `abc`.
- Output: the shortest palindrome whose prefix is the given string.
- Trace: a full TM execution trace (frames with tape and head) is written to `output.txt`.
- Modes: run step-by-step (interactive) or automatically.

## How It Works (TM Model)
The simulator uses a large tape of blanks (`_`), an indexable head, and textual state labels for clarity. It performs three conceptual phases using simple moves and read/write actions.

- Tape alphabet: `a..z` plus blank `_` (internally also uses temporary uppercase letters to mark visited cells during copying).
- Input placement: input `s` is written contiguously starting at `TAPE_START`.
- Extra space: a gap of blanks separates the input and a safe "reversed copy" region to avoid overwrite while copying.

Key constants (defined in code):
- `TAPE_SIZE = 8000`
- `TAPE_START = 1500`
- `COPY_GAP = 80` (gap between the end of input and start of the reversed copy)
- `MAX_INPUT = 1000`

### Phase 1 — Copy Reverse Safely
Goal: produce `rev(s)` to the right of the input without clobbering the source.

- The TM repeatedly seeks the rightmost unmarked lowercase character in the input region.
- It marks that character by writing its uppercase form in place (e.g., `a` → `A`).
- It then writes the lowercase version of that character at the current append position in the copy region. This builds `rev(s)` left-to-right.
- After all input characters are copied, the TM unmarks the input (converts uppercase back to lowercase).

Representative state labels in this phase: `q_copy_reverse_start`, `q_seek_rightmost_unmarked`, `q_mark_original`, `q_write_copy`, `q_copy_reverse_done`, `q_unmark_originals`.

### Phase 2 — Find Longest Overlap
Goal: compute the largest `t` (0 ≤ t ≤ |s|) such that the suffix of `s` of length `t` equals the prefix of `rev(s)` of length `t`.

Intuition: If the end of `s` already matches the beginning of `rev(s)`, we need to append fewer characters to make a palindrome.

- The TM tries `t = |s|, |s|-1, ..., 0`.
- For each candidate `t`, it compares characters pairwise: `s[|s|-t + i]` with `rev(s)[i]` for `i = 0..t-1`.
- It moves the head back and forth to read each cell and checks equality. On full match, it halts this phase with that `t`.

Representative state labels: `q_find_overlap_start`, `q_read_orig`, `q_read_copy`, `q_match_char`, `q_mismatch`, `q_overlap_found_t=...`.

### Phase 3 — Append Remainder to Form Palindrome
Goal: write the remaining tail of `rev(s)` after the matched prefix so that the combined string becomes a palindrome.

- Let `k = t` from Phase 2.
- The TM appends `rev(s)[k..|s|-1]` immediately to the right of the input.
- The final tape segment from `input_start` to the last written cell is the shortest palindrome with prefix `s`.

Representative state labels: `q_append_remainder_k=...`, `q_write_remainder`, `q_after_append`, `q_halt`.

### Why This Yields the Shortest Palindrome
Let `s` be the input and `rev(s)` its reverse. The largest overlap `t` ensures the longest suffix of `s` matches the longest prefix of `rev(s)`. Appending only the remainder `rev(s)[t..]` is the minimum extension that completes the palindrome: the left half (from `s`) mirrors the newly appended right half.

## Interaction and Output
- On start, the program prints a banner and asks for input.
- It then prompts: `Run interactively? (y = step-by-step, n = automatic)`.
  - `y` (or Enter): step-by-step. You press Enter to advance; type `a` then Enter once to switch to automatic mid-run.
  - `n`: automatic run (still prints concise progress to console).
- Detailed frames are written to `output.txt`:
  - `step <#> | state: <label>`
  - `Tape: <window-of-cells>`
  - A caret `^` marks the head position beneath the tape.
- The console shows a concise summary per step and the final result string with total steps.

## Usage
1. Compile the code using a C compiler (e.g., `gcc turing_mac.c -o turing_mac`).
2. Run the executable (e.g., `./turing_mac` on Unix or `turing_mac.exe` on Windows).
3. Follow the prompts to enter a lowercase string and choose the run mode (interactive or automatic).