#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Run fortdb with input commands from a temp file, capture its output into another temp file,
   and check whether expected_substr appears in the output. */
static int run_test(const char *label, const char *commands, const char *expected_substr) {
    char in_template[] = "tmp_in_XXXXXX";
    char out_template[] = "tmp_out_XXXXXX";
    int in_fd = mkstemp(in_template);
    if (in_fd < 0) {
        fprintf(stderr, "%s: mkstemp(in): %s\n", label, strerror(errno));
        return 1;
    }

    ssize_t wrote = write(in_fd, commands, (size_t)strlen(commands));
    if (wrote < 0) {
        fprintf(stderr, "%s: write(in): %s\n", label, strerror(errno));
        close(in_fd);
        unlink(in_template);
        return 1;
    }
    close(in_fd);

    int out_fd = mkstemp(out_template);
    if (out_fd < 0) {
        fprintf(stderr, "%s: mkstemp(out): %s\n", label, strerror(errno));
        unlink(in_template);
        return 1;
    }
    close(out_fd);

    /* Build and run command */
    char cmd[512];
    int n = snprintf(cmd, sizeof(cmd), "../src/fortdb < %s > %s 2>&1", in_template, out_template);
    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "%s: command buffer overflow\n", label);
        unlink(in_template);
        unlink(out_template);
        return 1;
    }
    int sysret = system(cmd);
    (void)sysret; /* we will still inspect the output file */

    /* Read captured output */
    FILE *f = fopen(out_template, "r");
    if (!f) {
        printf("%s: UNIT TEST: FAILED (could not open output file)\n", label);
        unlink(in_template);
        unlink(out_template);
        return 1;
    }

    /* Slurp file content */
    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = NULL;
    if (flen > 0) {
        buf = malloc((size_t)flen + 1);
        if (!buf) {
            fclose(f);
            unlink(in_template);
            unlink(out_template);
            fprintf(stderr, "%s: malloc failed\n", label);
            return 1;
        }
        size_t r = fread(buf, 1, (size_t)flen, f);
        buf[r] = '\0';
    } else {
        buf = strdup("");
    }
    fclose(f);

    /* Print the output for inspection */
    printf("---- %s OUTPUT START ----\n", label);
    printf("%s\n", buf);
    printf("---- %s OUTPUT END   ----\n", label);

    /* Check expectation */
    int passed = 0;
    if (expected_substr != NULL && strstr(buf, expected_substr) != NULL) passed = 1;

    if (passed) {
        printf("%s: UNIT TEST: PASSED\n\n", label);
    } else {
        printf("%s: UNIT TEST: FAILED\n\n", label);
    }

    free(buf);
    unlink(in_template);
    unlink(out_template);
    return passed ? 0 : 1;
}

int main(void) {
    int total = 0;
    int failed = 0;

    /* Test: load */
    total++;
    if (run_test("SETUP: LOAD", "load db.fort\nexit\n", "Loaded database from db.fort") != 0) failed++;

    /* Test: set + get */
    total++;
    if (run_test("SET FIELD", "set users/alice/email alice@example.com\nexit\n", "OK\n") != 0) {
        failed++;
    } else {
        /* If set passed, test get */
        total++;
        if (run_test("GET FIELD", "get users/alice/email\nexit\n", "string:alice@example.com") != 0) failed++;
    }

    /* Test: list-versions (two sets then list) */
    total++;
    if (run_test("LIST-VERSIONS", "set users/alice/age 25\nset users/alice/age 30\nlist-versions users/alice/age\nexit\n", "v2: 30") != 0) failed++;

    /* Test: compact */
    total++;
    if (run_test("COMPACT", "compact users/alice\nexit\n", "Compacted users/alice") != 0) failed++;

    /* Test: save */
    total++;
    if (run_test("SAVE", "save db.fort\nexit\n", "Saved database to db.fort") != 0) failed++;

    int passed = total - failed;
    printf("Passed: %d / %d\n", passed, total);
    printf("Failed: %d\n", failed);

    /* Exit code non-zero if any failed */
    return (failed == 0) ? 0 : 1;
}

