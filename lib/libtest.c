#include <fcntl.h>
#include <lib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int get_file(const char *functionName) {
  char testName[100];
  snprintf(testName, 100, "output/%s.txt", functionName);
  return open(testName, O_CREAT | O_WRONLY, 0644);
}

void basic_test() {
  State *state = create_state(20, 6);
  int fd = get_file(__func__);

  char *input = "Hello, World!\n";
  send_input(state, input, strlen(input));
  print_cells(fd, state);

  close(fd);
}

void wrap_test() {
  State *state = create_state(20, 6);
  int fd = get_file(__func__);

  char *input = "This text is longer than one line.";
  send_input(state, input, strlen(input));
  print_cells(fd, state);

  close(fd);
}

void scroll_test() {
  State *state = create_state(20, 6);
  int fd = get_file(__func__);

  char *input = "This text is longer than one line.\n";
  for (int i = 0; i < 10; i++) {
    send_input(state, input, strlen(input));
  }
  print_cells(fd, state);

  close(fd);
}

void color_test() {
  State *state = create_state(20, 6);
  int fd = get_file(__func__);

  char *input = "Hello, World!\n";
  send_input(state, input, strlen(input));
  for(int i=0; i<strlen("Hello"); i++){
    state->cells[i].fg.type = COLOR_TYPE_INDEX;
    state->cells[i].fg.index = 1;
  }

  for(int i=7; i<7+strlen("World"); i++){
    state->cells[i].bg.type = COLOR_TYPE_INDEX;
    state->cells[i].bg.index = 2;
  }

  print_cells(fd, state);

  close(fd);
}

void stress_test() {
  State *state = create_state(20, 6);
  int fd = get_file(__func__);

  char *input = "This text is longer than one line.\n";
  for (int i = 0; i < 10000; i++) {
    send_input(state, input, strlen(input));
  }
  print_cells(fd, state);

  close(fd);
}

int main() {
  basic_test();
  wrap_test();
  scroll_test();
  color_test();
  stress_test();
}
