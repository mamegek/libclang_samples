#include <stdio.h>

int add(int a, int b) {
printf(">> add called (a=%d, b=%d)\n", a, b);
 return a + b; }

void print_message(const char *msg) {
printf(">> print_message called (msg=%s)\n", msg);
 printf("Message: %s\n", msg); }

int factorial(int n) {
printf(">> factorial called (n=%d)\n", n);

  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

double calculate_average(int *array, int size) {
printf(">> calculate_average called (array=%p, size=%d)\n", array, size);

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
printf(">> swap called (a=%p, b=%p)\n", a, b);

  int temp = *a;
  *a = *b;
  *b = temp;
}

int main() {
printf(">> main called\n");

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
