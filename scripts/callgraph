#!/bin/bash

make || exit 1

(
echo "strict digraph G {"
echo "rankdir=LR"
echo "node [shape=box style=filled fillcolor=lightblue]"

# Background for entire graph
echo "bgcolor=\"#f0f0f0\""

objdump -d build/ntmux | awk '
BEGIN {
  init=""
}

/get_current_session/ { next }
/get_current_window/ { next }
/get_current_pane/ { next }
/@/ { next }
/\./ { next }
/<_/ { next }
/+/ { next }
!/</ { next }
/atexit/ { next }
/tm_clones/ { next }

/^[0-9]/ {
  gsub(/.*<|>.*/, "")
  init=$0
}

/call/ {
  gsub(/.*<|>.*/, "")
  if (init == $0) {
    next
  }
  printf "\"" init "\"->\"" $0 "\"\n"
}' | sort | uniq

# Custom edges
echo "$1"
echo "start_client->receive_messages"
echo "start_server_loop->handle_client"
echo "initialize_vterm_instance->handle_term_prop"
echo "initialize_vterm_instance->handle_push_line"
echo "receive_messages->handle_resize"
echo "start_client->handle_sigint"
echo "main->handle_ctrl_c"
echo "handle_events->add_readline_history"
echo "start_server_loop->handle_cleanup"
echo "write_message->read_message[style=dotted]"

function render_module() {
  module="$1"

  echo "subgraph cluster_$module {"
  nm "build/$module.o" | grep "^[0-9a-f]\+ T " | cut -f3 -d' '
  echo "bgcolor=lightgrey"
  echo "label=\"$module module\""
  echo "}"
}

#echo "subgraph cluster_c_main {"
#echo "label=\"Main\""
#echo 'bgcolor="#ddcccc"'
render_module "args"
render_module "main"
#echo "}"

#echo "subgraph cluster_c_commands {"
#echo "label=\"Commands\""
#echo 'bgcolor="#ccccdd"'
render_module "command"
render_module "layout"
render_module "move"
render_module "pane"
render_module "plugin"
render_module "print_session"
render_module "pty"
#echo "}"

#echo "subgraph cluster_c_render {"
#echo "label=\"Render\""
#echo 'bgcolor="#ccddcc"'
render_module "border"
render_module "render"
render_module "render_cell"
render_module "statusbar"
#echo "}"

#echo "subgraph cluster_c_comms {"
#echo "label=\"Comms\""
#echo 'bgcolor="#ddccdd"'
render_module "client"
render_module "connection"
render_module "mouse"
render_module "server"
#echo "}"

echo "}"
) > /tmp/callgraph.dot

dot -Tpng < /tmp/callgraph.dot | feh --scale-down -
