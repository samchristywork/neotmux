local util = require("bindings.util")
local Command = util.Command
local Event = util.Event

local alt = "\x1b"

bindings = {}
bindings.normal = {}
bindings.control = {}

function add_binding(mode, key, value, docs)
  if mode == "n" then
    bindings.normal[key] = {}
    bindings.normal[key].command = value
    bindings.normal[key].docs = docs
  elseif mode == "c" then
    bindings.control[key] = {}
    bindings.control[key].command = value
    bindings.control[key].docs = docs
  end
end

-- Directional
add_binding("c", "H", Command("SwapLeft"), "Swap pane left")
add_binding("c", "J", Command("SwapDown"), "Swap pane down")
add_binding("c", "K", Command("SwapUp"), "Swap pane up")
add_binding("c", "L", Command("SwapRight"), "Swap pane right")
add_binding("c", "\x1b[A", Command("Up"), "Move focus up")
add_binding("c", "\x1b[B", Command("Down"), "Move focus down")
add_binding("c", "\x1b[C", Command("Right"), "Move focus right")
add_binding("c", "\x1b[D", Command("Left"), "Move focus left")
add_binding("c", "h", Command("Left"), "Move focus left")
add_binding("c", "j", Command("Down"), "Move focus down")
add_binding("c", "k", Command("Up"), "Move focus up")
add_binding("c", "l", Command("Right"), "Move focus right")
add_binding("n", alt.."H", Command("SwapLeft"), "Swap pane left")
add_binding("n", alt.."J", Command("SwapDown"), "Swap pane down")
add_binding("n", alt.."K", Command("SwapUp"), "Swap pane up")
add_binding("n", alt.."L", Command("SwapRight"), "Swap pane right")
add_binding("n", alt.."h", Command("Left"), "Move focus left")
add_binding("n", alt.."j", Command("Down"), "Move focus down")
add_binding("n", alt.."k", Command("Up"), "Move focus up")
add_binding("n", alt.."l", Command("Right"), "Move focus right")

-- Layout
add_binding("c", "1", Command("Layout layout_even_horizontal"), "Even horizontal layout")
add_binding("c", "2", Command("Layout layout_even_vertical"), "Even vertical layout")
add_binding("c", "3", Command("Layout layout_main_horizontal"), "Main horizontal layout")
add_binding("c", "4", Command("Layout layout_main_vertical"), "Main vertical layout")
add_binding("c", "5", Command("Layout layout_tiled"), "Tiled layout")
add_binding("c", "6", Command("Layout layout_custom"), "Custom layout")
add_binding("n", alt.."1", Command("Layout layout_even_horizontal"), "Even horizontal layout")
add_binding("n", alt.."2", Command("Layout layout_even_vertical"), "Even vertical layout")
add_binding("n", alt.."3", Command("Layout layout_main_horizontal"), "Main horizontal layout")
add_binding("n", alt.."4", Command("Layout layout_main_vertical"), "Main vertical layout")
add_binding("n", alt.."5", Command("Layout layout_tiled"), "Tiled layout")
add_binding("n", alt.."6", Command("Layout layout_custom"), "Custom layout")

-- Misc.
add_binding("n", "\x0a", Event("\x0d"), nil) -- Convert newline to carriage return
add_binding("n", alt.."z", Command("Zoom"), "Zoom window")

-- Splits
-- TODO: There is currently no distinction between horizontal and vertical splits
add_binding("c", "|", Command("Split"), "Split horizontally")
add_binding("c", "_", Command("VSplit"), "Split vertically")
add_binding("c", "\"", Command("Split"), "Split horizontally")
add_binding("c", "%", Command("VSplit"), "Split vertically")

-- Window
add_binding("c", "e", Command("Create"), "Create window")

-- Misc.
add_binding("c", "\x01", Event("\x01"), nil) -- Ctrl + A
add_binding("c", "\x02", Event("ls\n"), nil) -- Ctrl + B
add_binding("c", "i", Command("List"), "List client data")
add_binding("c", "q", Command("Quit"), "Quit")
add_binding("c", "r", Command("ReloadLua"), "Reload Lua")
add_binding("c", "x", Command("CopySelection"), "Copy selection")
add_binding("c", "y", Command("CycleStatus"), "Cycle status")
add_binding("c", "z", Command("Zoom"), "Zoom pane")

add_binding("n", alt.."n", function(sock)
  write_string(sock, Command("Next"))
  send_size(sock)
  write_string(sock, Command("ReRender"));
  write_string(sock, Command("RenderBar"));
end, "Next window")

add_binding("n", alt.."p", function(sock)
  write_string(sock, Command("Prev"))
  send_size(sock)
  write_string(sock, Command("ReRender"));
  write_string(sock, Command("RenderBar"));
end, "Previous window")

add_binding("c", "n", function(sock)
  write_string(sock, Command("Next"))
  send_size(sock)
  write_string(sock, Command("ReRender"));
  write_string(sock, Command("RenderBar"));
end, "Next window")

add_binding("c", "p", function(sock)
  write_string(sock, Command("Prev"))
  send_size(sock)
  write_string(sock, Command("ReRender"));
  write_string(sock, Command("RenderBar"));
end, "Previous window")

function run_script(script)
  local f = io.popen(script)
  local lines = {}
  for line in f:lines() do
    table.insert(lines, line)
  end
  f:close()
  return lines
end

add_binding("c", "?", function(sock)
  reset_mode()
  local lines = run_script("./scripts/help.sh")
  for i, line in ipairs(lines) do
    write_string(sock, "c" .. line)
  end
  enter_raw_mode()
end, "Display help")

add_binding("c", "g", function(sock)
  run_script("clear")
  reset_mode()
  local lines = run_script("./scripts/show_log.sh")
  for i, line in ipairs(lines) do
    write_string(sock, line)
  end
  enter_raw_mode()
end, "Show log")

add_binding("c", "o", function(sock)
  reset_mode()
  local lines = run_script("./scripts/list_commands.sh")
  for i, line in ipairs(lines) do
    write_string(sock, line)
  end
  enter_raw_mode()
end, "Select command from a list")

-- TODO: Page Up and Page Down

util.print_unused_bindings()
util.list_docs()
