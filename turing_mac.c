#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAPE_SIZE 8000
#define TAPE_START 1500
#define COPY_GAP 80
#define MAX_INPUT 1000

char tape[TAPE_SIZE];
int head;
char state[120];
int step_count = 0;
int verbose = 1;
int interactive_mode = 1;

int input_start, input_end;
int copy_start, copy_end;

FILE *out = NULL;

#define OUT_PRINTF(...) do { if (out) { fprintf(out, __VA_ARGS__); fflush(out); } } while(0)
#define OUT_PUTC(ch) do { if (out) { fputc((ch), out); fflush(out); } } while(0)
#define OUT_PUTS(s) do { if (out) { fputs((s), out); fputc('\n', out); fflush(out); } } while(0)

void init_tape() {
    for (int i = 0; i < TAPE_SIZE; i++) tape[i] = '_';
}

void print_tape_frame(int left, int right) {
    if (!out) return;
    if (left < 0) left = 0;
    if (right >= TAPE_SIZE) right = TAPE_SIZE - 1;

    OUT_PRINTF("\nstep %d | state: %s\n", step_count, state);
    OUT_PRINTF("Tape: ");
    for (int i = left; i <= right; i++) OUT_PUTC(tape[i]);
    OUT_PUTC('\n');

    OUT_PRINTF("      ");
    for (int i = left; i <= right; i++) {
        if (i == head) OUT_PUTC('^'); else OUT_PUTC(' ');
    }
    OUT_PUTC('\n');

    printf("step %d | state: %s | head=%d (see output.txt for full tape)\n", step_count, state, head);
    fflush(stdout);
}

void step_pause() {
    if (!verbose) return;

    if (!interactive_mode) {
        printf("step %d | state: %s | head=%d (see output.txt for full tape)\n", step_count, state, head);
        fflush(stdout);
        return;
    }

    printf("Press Enter for next action (type 'a' then Enter to run automatic): ");
    fflush(stdout);

    if (out) { fprintf(out, "[PROMPT] Press Enter for next action (or 'a' to auto)\n"); fflush(out); }

    char buf[32];
    if (!fgets(buf, sizeof(buf), stdin)) return;
    if (buf[0] == 'a' || buf[0] == 'A') {
        interactive_mode = 0;
        if (out) { fprintf(out, "[PROMPT-RESPONSE] automatic mode enabled\n"); fflush(out); }
    } else {
        if (out) { fprintf(out, "[PROMPT-RESPONSE] step-by-step continue\n"); fflush(out); }
    }
}


void write_symbol(char sym) { tape[head] = sym; }
char read_symbol() { return tape[head]; }
void move_left() {
    if (head == 0) { fprintf(stderr, "Head moved beyond left end!\n"); exit(1); }
    head--;
}
void move_right() {
    if (head == TAPE_SIZE - 1) { fprintf(stderr, "Head moved beyond right end!\n"); exit(1); }
    head++;
}

void show_tape_window() {
    int left = input_start - 12;
    int right = copy_end + 12;
    if (left < 0) left = 0;
    if (right >= TAPE_SIZE) right = TAPE_SIZE - 1;
    print_tape_frame(left, right);
}

void error_and_exit(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    if (out) {
        fprintf(out, "Error: %s\n", msg);
        fflush(out);
    }
    exit(1);
}

void tm_copy_reverse() {
    strcpy(state, "q_copy_reverse_start"); step_count++; show_tape_window(); step_pause();

    int append_pos = copy_start;
    copy_end = append_pos - 1;

    while (1) {
        int found = -1;
        for (int p = input_end; p >= input_start; p--) {
            if (isalpha((unsigned char)tape[p]) && islower((unsigned char)tape[p])) { found = p; break; }
        }
        strcpy(state, "q_seek_rightmost_unmarked"); step_count++; show_tape_window(); step_pause();

        if (found == -1) {
            strcpy(state, "q_copy_reverse_done"); step_count++; show_tape_window(); step_pause();
            break;
        }

        while (head < found) { move_right(); step_count++; strcpy(state, "move_right"); show_tape_window(); step_pause(); }
        while (head > found) { move_left(); step_count++; strcpy(state, "move_left"); show_tape_window(); step_pause(); }

        char ch = read_symbol();
        if (!(isalpha((unsigned char)ch) && islower((unsigned char)ch))) error_and_exit("expected lowercase at found");
        write_symbol((char) toupper(ch));
        step_count++; strcpy(state, "q_mark_original"); show_tape_window(); step_pause();

        while (head < append_pos) { move_right(); step_count++; strcpy(state, "move_right_to_append"); show_tape_window(); step_pause(); }
        while (head > append_pos) { move_left(); step_count++; strcpy(state, "move_left_to_append"); show_tape_window(); step_pause(); }

        write_symbol(ch);
        step_count++; strcpy(state, "q_write_copy"); show_tape_window(); step_pause();

        append_pos++;
        copy_end = append_pos - 1;

        head = input_end;
        step_count++; strcpy(state, "q_return_search"); show_tape_window(); step_pause();
    }

    strcpy(state, "q_unmark_originals"); step_count++; show_tape_window(); step_pause();
    for (int p = input_start; p <= input_end; p++) {
        if (isalpha((unsigned char)tape[p]) && isupper((unsigned char)tape[p])) {
            head = p;
            write_symbol((char) tolower(tape[p]));
            step_count++; strcpy(state, "q_unmark_cell"); show_tape_window(); step_pause();
        }
    }
    head = input_start; step_count++; strcpy(state, "q_after_unmark"); show_tape_window(); step_pause();
}

int tm_find_longest_overlap() {
    strcpy(state, "q_find_overlap_start"); step_count++; show_tape_window(); step_pause();
    int n = input_end - input_start + 1;
    if (n == 0) return 0;

    for (int t = n; t >= 0; t--) {
        int ok = 1;
        if (t == 0) ok = 1;
        else {
            for (int i = 0; i < t; i++) {
                int pos_orig = input_end - t + 1 + i;
                int pos_copy = copy_start + i;

                while (head < pos_orig) { move_right(); step_count++; strcpy(state, "move_right_cmp"); show_tape_window(); step_pause(); }
                while (head > pos_orig) { move_left(); step_count++; strcpy(state, "move_left_cmp"); show_tape_window(); step_pause(); }
                char ch1 = read_symbol(); step_count++; strcpy(state, "q_read_orig"); show_tape_window(); step_pause();

                while (head < pos_copy) { move_right(); step_count++; strcpy(state, "move_right_cmp2"); show_tape_window(); step_pause(); }
                while (head > pos_copy) { move_left(); step_count++; strcpy(state, "move_left_cmp2"); show_tape_window(); step_pause(); }
                char ch2 = read_symbol(); step_count++; strcpy(state, "q_read_copy"); show_tape_window(); step_pause();

                if (ch1 != ch2) { ok = 0; step_count++; strcpy(state, "q_mismatch"); show_tape_window(); step_pause(); break; }
                else { step_count++; strcpy(state, "q_match_char"); show_tape_window(); step_pause(); }
            }
        }
        if (ok) {
            char buf[120]; sprintf(buf, "q_overlap_found_t=%d", t);
            strcpy(state, buf); step_count++; show_tape_window(); step_pause();
            return t;
        } else {
            char buf[120]; sprintf(buf, "q_try_lower_t=%d", t-1);
            strcpy(state, buf); step_count++; show_tape_window(); step_pause();
        }
    }
    return 0;
}

void tm_append_remainder(int k) {
    char buf[120]; sprintf(buf, "q_append_remainder_k=%d", k); strcpy(state, buf); step_count++; show_tape_window(); step_pause();

    int n = input_end - input_start + 1;
    int write_pos = input_end + 1;

    for (int i = k; i < n; i++) {
        int from = copy_start + i;
        while (head < from) { move_right(); step_count++; strcpy(state, "move_right_to_from"); show_tape_window(); step_pause(); }
        while (head > from) { move_left(); step_count++; strcpy(state, "move_left_to_from"); show_tape_window(); step_pause(); }
        char ch = read_symbol();

        while (head < write_pos) { move_right(); step_count++; strcpy(state, "move_right_to_write"); show_tape_window(); step_pause(); }
        while (head > write_pos) { move_left(); step_count++; strcpy(state, "move_left_to_write"); show_tape_window(); step_pause(); }

        write_symbol(ch);
        step_count++; strcpy(state, "q_write_remainder"); show_tape_window(); step_pause();

        write_pos++;
        if (write_pos - 1 > copy_end) copy_end = write_pos - 1;
    }

    strcpy(state, "q_after_append"); step_count++; show_tape_window(); step_pause();
}

void print_header() {
    printf("  ____                  _ _           _               \n");
    printf(" / ___| _ __ ___   __ _| | | ___  ___| |_             \n");
    printf(" \\___ \\| '_ ` _ \\ / _` | | |/ _ \\/ __| __|            \n");
    printf("  ___) | | | | | | (_| | | |  __/\\__ \\ |_             \n");
    printf(" |____/|_| |_| |_|\\__,_|_|_|\\___||___/\\__|            \n");
    printf(" |  _ \\ __ _| (_)_ __   __| |_ __ ___  _ __ ___   ___ \n");
    printf(" | |_) / _` | | | '_ \\ / _` | '__/ _ \\| '_ ` _ \\ / _ \\\n");
    printf(" |  __/ (_| | | | | | | (_| | | | (_) | | | | | |  __/\n");
    printf(" |_|___\\__,_|_|_|_| |_|\\__,_|_|  \\___/|_| |_| |_|\\___|\n");
    printf("  / ___| ___ _ __   ___ _ __ __ _| |_ ___  _ __       \n");
    printf(" | |  _ / _ \\ '_ \\ / _ \\ '__/ _` | __/ _ \\| '__|      \n");
    printf(" | |_| |  __/ | | |  __/ | | (_| | || (_) | |         \n");
    printf("  \\____|\\___|_| |_|\\___|_|  \\__,_|\\__\\___/|_|         \n");
    printf("                                                  \n");
}

int main() {
    init_tape();
    char input[MAX_INPUT+1];

    print_header();

    out = fopen("output.txt", "w");
    if (!out) {
        fprintf(stderr, "Failed to open output.txt for writing\n");
        return 1;
    }

    printf("Turing Machine: produce smallest palindrome with given prefix\n");
    printf("All TM trace output will be written to 'output.txt'.\n");
    printf("Input alphabet: lowercase letters (a..z). Example: abc\n");
    printf("Enter input prefix: ");
    fflush(stdout);

    OUT_PRINTF("Turing Machine: produce smallest palindrome with given prefix\n");
    OUT_PRINTF("Input alphabet: lowercase letters (a..z). Example: abc\n");

    if (scanf("%1000s", input) != 1) {
        fclose(out);
        return 0;
    }

    {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }

    printf("Run interactively? (y = step-by-step, n = automatic): ");
    fflush(stdout);
    OUT_PRINTF("[PROMPT] Run interactively? (y = step-by-step, n = automatic) [y]\n");

    char choicebuf[32];
    if (fgets(choicebuf, sizeof(choicebuf), stdin) != NULL) {
        char *p = choicebuf;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == 'n' || *p == 'N') { interactive_mode = 0; verbose = 1; }
        else { interactive_mode = 1; verbose = 1; }
    } else {
        interactive_mode = 1; verbose = 1;
    }

    OUT_PRINTF("[CHOICE] interactive_mode=%d\n", interactive_mode);

    int n = (int) strlen(input);
    if (n == 0) {
        printf("Empty input -> empty palindrome\n");
        OUT_PUTS("Empty input -> empty palindrome");
        fclose(out);
        return 0;
    }
    if (n > MAX_INPUT) {
        printf("Input too long\n");
        OUT_PUTS("Input too long");
        fclose(out);
        return 0;
    }

    input_start = TAPE_START;
    for (int i = 0; i < n; i++) tape[input_start + i] = input[i];
    input_end = input_start + n - 1;

    copy_start = input_end + 1 + COPY_GAP;
    copy_end = input_end;

    head = input_start;
    strcpy(state, "q_start");
    step_count = 0;
    show_tape_window();

    int already_pal = 1;
    for (int i = 0; i < n/2; i++) {
        if (tape[input_start + i] != tape[input_end - i]) { already_pal = 0; break; }
    }
    if (already_pal) {
        printf("\nInput is already a palindrome. Result: ");
        for (int i = input_start; i <= input_end; i++) putchar(tape[i]);
        putchar('\n');

        OUT_PRINTF("\nInput is already a palindrome. Result: ");
        for (int i = input_start; i <= input_end; i++) OUT_PUTC(tape[i]);
        OUT_PUTC('\n');

        fclose(out);
        return 0;
    }

    tm_copy_reverse();

    int k = tm_find_longest_overlap();

    tm_append_remainder(k);

    strcpy(state, "q_halt"); step_count++; show_tape_window();

    OUT_PRINTF("\nTM halted. Final tape content (from input start to copy end):\n");
    for (int i = input_start; i <= copy_end; i++) OUT_PUTC(tape[i]);
    OUT_PUTC('\n');
    OUT_PRINTF("Total TM steps (simulated head actions + labeled actions): %d\n", step_count);

    printf("\nTM halted. Final result written to output.txt (from input start to copy end):\n");
    for (int i = input_start; i <= copy_end; i++) putchar(tape[i]);
    putchar('\n');
    printf("Total TM steps: %d\n", step_count);

    fclose(out);
    return 0;
}
