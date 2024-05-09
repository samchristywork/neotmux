#!/bin/bash

(
echo "digraph G {"
echo "rankdir=LR"
echo "node [shape=box]"

objdump -d build/ntmux | awk '
BEGIN {
  init=""
}

/@/ { next }
/\./ { next }
/<_/ { next }
/+/ { next }
!/</ { next }
/tm_clones/ { next }

/^[0-9]/ {
  gsub(/.*<|>.*/, "")
  init=$0
}

/call/ {
  gsub(/.*<|>.*/, "")
  printf "\"" init "\"->\"" $0 "\"\n"
}' | sort | uniq

echo "}"
) > /tmp/callgraph.dot

dot -Tpng < /tmp/callgraph.dot | feh --scale-down -
