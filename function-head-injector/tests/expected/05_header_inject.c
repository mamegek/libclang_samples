/* FHI_INJECT_BEGIN */
/* 
 * This is an injected header file 
 * It will be inserted at the top.
 */
#include <unistd.h>
/* FHI_INJECT_END */
#include <stdio.h>

int add(int a, int b) {
/* FHI_INJECT_BEGIN */
/******** DEBUG CODE ********/
printf(">> Entering function at %s:%d\n", __FILE__, __LINE__);
/******** DEBUG CODE ********/
/* FHI_INJECT_END */
 return a + b; }

void print_message(const char *msg) {
/* FHI_INJECT_BEGIN */
/******** DEBUG CODE ********/
printf(">> Entering function at %s:%d\n", __FILE__, __LINE__);
/******** DEBUG CODE ********/
/* FHI_INJECT_END */
 printf("Message: %s\n", msg); }

int factorial(int n) {
/* FHI_INJECT_BEGIN */
/******** DEBUG CODE ********/
printf(">> Entering function at %s:%d\n", __FILE__, __LINE__);
/******** DEBUG CODE ********/
/* FHI_INJECT_END */

  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

double calculate_average(int *array, int size) {
/* FHI_INJECT_BEGIN */
/******** DEBUG CODE ********/
printf(">> Entering function at %s:%d\n", __FILE__, __LINE__);
/******** DEBUG CODE ********/
/* FHI_INJECT_END */

  if (size == 0) {
    return 0.0;
  }

  int sum = 0;
  for (int i = 0; i < size; i++) {
    sum += array[i];
  }

  return (double)sum / size;
}

void swap(int *a, int *b) {
/* FHI_INJECT_BEGIN */
/******** DEBUG CODE ********/
printf(">> Entering function at %s:%d\n", __FILE__, __LINE__);
/******** DEBUG CODE ********/
/* FHI_INJECT_END */

  int temp = *a;
  *a = *b;
  *b = temp;
}

int main() {
/* FHI_INJECT_BEGIN */
/******** DEBUG CODE ********/
printf(">> Entering function at %s:%d\n", __FILE__, __LINE__);
/******** DEBUG CODE ********/
/* FHI_INJECT_END */

  printf("Testing functions\n");

  int result = add(5, 3);
  printf("5 + 3 = %d\n", result);

  print_message("Hello, World!");

  int fact = factorial(5);
  printf("5! = %d\n", fact);

  int numbers[] = {1, 2, 3, 4, 5};
  double avg = calculate_average(numbers, 5);
  printf("Average: %.2f\n", avg);

  int x = 10, y = 20;
  printf("Before swap: x=%d, y=%d\n", x, y);
  swap(&x, &y);
  printf("After swap: x=%d, y=%d\n", x, y);

  return 0;
}
