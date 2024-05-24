local util = require("bindings.util")
local Command = util.Command
local Event = util.Event

local alt = "\x1b"

function add_binding(mode, key, value, docs)
  local normal = bindings["normal"]
  local control = bindings["control"]
  if mode == "n" then
    normal[key] = {}
    normal[key].command = value
    normal[key].docs = docs
  elseif mode == "c" then
    control[key] = {}
    control[key].command = value
    control[key].docs = docs
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
add_binding("n", alt.."n", Command("Next"), "Next window")
add_binding("n", alt.."p", Command("Prev"), "Previous window")
add_binding("n", alt.."z", Command("Zoom"), "Zoom window")

-- Splits
-- TODO: There is currently no distinction between horizontal and vertical splits
add_binding("c", "|", Command("Split"), "Split horizontally")
add_binding("c", "_", Command("VSplit"), "Split vertically")
add_binding("c", "\"", Command("Split"), "Split horizontally")
add_binding("c", "%", Command("VSplit"), "Split vertically")

-- Window
add_binding("c", "e", Command("Create"), "Create window")
add_binding("c", "n", Command("Next"), "Next window")
add_binding("c", "p", Command("Prev"), "Previous window")

-- Misc.
add_binding("c", "\x01", Event("\x01"), nil) -- Ctrl + A
add_binding("c", "\x02", Event("ls\n"), nil) -- Ctrl + B
add_binding("c", "i", Command("List"), "List client data")
add_binding("c", "q", Command("Quit"), "Quit")
add_binding("c", "r", Command("ReloadLua"), "Reload Lua")
add_binding("c", "x", Command("CopySelection"), "Copy selection")
add_binding("c", "y", Command("CycleStatus"), "Cycle status")
add_binding("c", "z", Command("Zoom"), "Zoom pane")

add_binding("c", "?", function(sock)
  system("clear")
  reset_mode()
  system("./scripts/help.sh")
  enter_raw_mode()
end, "Display help")

add_binding("c", "g", function(sock)
  system("clear")
  reset_mode()
  system("./scripts/show_log.sh")
  enter_raw_mode()
end, "Show log")

-- TODO: Page Up and Page Down

util.print_unused_bindings()
util.list_docs()