#!/bin/bash

function print_all_colors {
  for i in {0..255}; do
    if (( $i % 16 == 0 )); then
      if (( $i != 0 )); then
        echo
      fi
    fi
    printf "\x1b[38;5;${i}m%03d " $i
  done
  printf "\x1b[0m"
  echo
}

function print_rgb_blue_gradient {
  start=0
  end=251
  step=4
  red=0
  green=0
  for blue in $(seq $start $step $end); do
    printf "\x1b[48;2;${red};${green};${blue}m "
  done
  printf "\x1b[0m"
  echo
}

time (
for i in {1..300}; do
  print_all_colors
  #print_rgb_blue_gradient
done
)
