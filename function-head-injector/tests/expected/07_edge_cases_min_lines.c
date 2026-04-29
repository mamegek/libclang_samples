/* FHI_INJECT_BEGIN */
/* FHI_INJECT_END */
#include <stdio.h>

// One-line function with body on same line
int add(int a, int b) { return a + b; }

// Empty function body
void noop() {}

// Empty body with spaces
void noop2() {  }

// Function with only a single return
int zero() {
    return 0;
}

// Nested braces
void nested() {
/* FHI_INJECT_BEGIN */
printf(">> nested called\n");
/* FHI_INJECT_END */

    if (1) {
        printf("inner\n");
    }
}

// Multi-line signature, body on next line
void long_signature(int a, int b,
                    int c, int d) {
    printf("%d %d %d %d\n", a, b, c, d);
}

// Function with no arguments and deep body
void deep() {
/* FHI_INJECT_BEGIN */
printf(">> deep called\n");
/* FHI_INJECT_END */

    {
        {
            printf("deep\n");
        }
    }
}

int main() {
/* FHI_INJECT_BEGIN */
printf(">> main called\n");
/* FHI_INJECT_END */

    add(1, 2);
    noop();
    noop2();
    zero();
    nested();
    long_signature(1, 2, 3, 4);
    deep();
    return 0;
}
