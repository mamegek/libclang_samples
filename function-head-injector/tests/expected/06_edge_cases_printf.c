#include <stdio.h>

// One-line function with body on same line
int add(int a, int b) {printf(">> add called (a=%d, b=%d)\n", a, b); return a + b; }

// Empty function body
void noop() {printf(">> noop called\n");}

// Empty body with spaces
void noop2() {printf(">> noop2 called\n");  }

// Function with only a single return
int zero() {
    printf(">> zero called\n");

    return 0;
}

// Nested braces
void nested() {
    printf(">> nested called\n");

    if (1) {
        printf("inner\n");
    }
}

// Multi-line signature, body on next line
void long_signature(int a, int b,
                    int c, int d) {
    printf(">> long_signature called (a=%d, b=%d, c=%d, d=%d)\n", a, b, c, d);

    printf("%d %d %d %d\n", a, b, c, d);
}

// Function with no arguments and deep body
void deep() {
    printf(">> deep called\n");

    {
        {
            printf("deep\n");
        }
    }
}

int main() {
    printf(">> main called\n");

    add(1, 2);
    noop();
    noop2();
    zero();
    nested();
    long_signature(1, 2, 3, 4);
    deep();
    return 0;
}
