#!/bin/bash

printf "c$(awk '
BEGIN {
  OFS = " "
}
/^\/\// {
  category = $0
}
/\/\// {
  doc = $0
}
/memcmp|strcmp/ {
  match($0, /"([^"]+)"/, a)
  print a[1], category, doc
}' src/command.c | \
  sort -u | \
  sed 's/\s\+\/\// |/g' | \
  sed 's/Category: //g' | \
  column -t -s '|' | \
  fzf | \
  sed 's/\s.\+//')"
