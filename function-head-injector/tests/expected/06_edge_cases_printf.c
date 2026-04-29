/* FHI_INJECT_BEGIN */
/* FHI_INJECT_END */
#include <stdio.h>

// One-line function with body on same line
int add(int a, int b) {
/* FHI_INJECT_BEGIN */
printf(">> add called (a=%d, b=%d)\n", a, b);
/* FHI_INJECT_END */
 return a + b; }

// Empty function body
void noop() {
/* FHI_INJECT_BEGIN */
printf(">> noop called\n");
/* FHI_INJECT_END */
}

// Empty body with spaces
void noop2() {
/* FHI_INJECT_BEGIN */
printf(">> noop2 called\n");
/* FHI_INJECT_END */
  }

// Function with only a single return
int zero() {
/* FHI_INJECT_BEGIN */
printf(">> zero called\n");
/* FHI_INJECT_END */

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
/* FHI_INJECT_BEGIN */
printf(">> long_signature called (a=%d, b=%d, c=%d, d=%d)\n", a, b, c, d);
/* FHI_INJECT_END */

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
