#!/bin/bash

make || exit 1

# Light mode
BG=#f0f0f0
NODEBG=#aaddff
CLUSTERBG=#cccccc
FG=#000000

## Dark mode
#BG=#111111
#NODEBG=#111144
#CLUSTERBG=#222222
#FG=#eeeeee

(
echo "strict digraph G {"
echo "rankdir=LR"
echo "compound=true"
echo "node [shape=box style=filled fillcolor=\"$NODEBG\" color=\"$FG\" fontcolor=\"$FG\"]"
echo "edge [color=\"$FG\"]"

# Background for entire graph
echo "bgcolor=\"$BG\""

objdump -d build/ntmux | awk '
BEGIN {
  init=""
}

/write_log/ { next }
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

function render_module() {
  module="$1"

  echo "subgraph cluster_$module {"
  nm "build/$module.o" | grep "^[0-9a-f]\+ T " | cut -f3 -d' '
  echo "bgcolor=\"$CLUSTERBG\""
  echo "label=\"$module module\" color=\"$FG\" fontcolor=\"$FG\""
  echo "}"
}

#echo "subgraph cluster_c_main {"
#echo "label=\"Main\""
#echo 'bgcolor="#ddcccc"'
#echo "}"

#echo "subgraph cluster_c_commands {"
#echo "label=\"Commands\""
#echo 'bgcolor="#ccccdd"'
#echo "}"

#echo "subgraph cluster_c_render {"
#echo "label=\"Render\""
#echo 'bgcolor="#ccddcc"'
#echo "}"

#echo "subgraph cluster_c_comms {"
#echo "label=\"Comms\""
#echo 'bgcolor="#ddccdd"'
#echo "}"

render_module "add"
render_module "args"
render_module "border"
render_module "client"
render_module "clipboard"
render_module "command"
render_module "connection"
render_module "layout"
render_module "main"
render_module "mouse"
render_module "move"
render_module "pane"
render_module "plugin"
render_module "print_session"
render_module "pty"
render_module "render"
render_module "render_cell"
render_module "render_pane"
render_module "render_row"
render_module "server"
render_module "statusbar"
render_module "utf8"

#echo "write_message->add_process_to_pane [style=invis]"
#echo "start_server->start_client [style=invis]"
#echo "draw_borders->print_cursor_shape [style=invis]"
#echo "client->server [style=invis]"
echo "usage->start_server [style=invis]"

# Custom edges
echo "$1"
echo "start_client->receive_messages"
echo "start_server_loop->handle_client"
echo "initialize_vterm_instance->handle_term_prop"
echo "initialize_vterm_instance->handle_push_line"
echo "receive_messages->handle_resize"
echo "start_client->handle_sigint"
echo "start_server->handle_ctrl_c"
echo "handle_events->add_readline_history"
echo "start_client->handle_events"
echo "start_server_loop->handle_cleanup"
echo "write_message->read_message[style=dashed weight=0]"
echo "render->receive_message[style=dashed weight=0]"
echo "register_functions->lua_write_string"
echo "register_functions->lua_enter_raw_mode"
echo "register_functions->lua_reset_mode"
echo "register_functions->lua_system"
echo "register_functions->lua_send_size"

echo "}"
) > /tmp/callgraph.dot

dot -Tpng < /tmp/callgraph.dot > /tmp/callgraph.png
feh --scale-down /tmp/callgraph.png
