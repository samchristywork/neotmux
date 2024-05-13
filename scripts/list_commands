#!/bin/bash

printf "c$(awk '
BEGIN {
  OFS = " "
}
/\/\// {
  doc = $0
}
/strcmp/ {
  match($0, /"([^"]+)"/, a)
  print a[1], doc
}
// {
  next
}
' src/command.c | \
  sort -u | \
  sed 's/\s\+\/\// |/g' | \
  column -t -s '|' | \
  fzf | \
  sed 's/\s.\+//')"
