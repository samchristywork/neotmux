local util = require("bindings.util")
local Command = util.Command
local Event = util.Event

-- NOTE: Must be global.
bindings = {
  ["control"] = {},
  ["normal"] = {}
}

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

-- Alt + hjkl
add_binding("n", alt.."h", Command("Left"), "Move focus left")
add_binding("n", alt.."j", function(sock) write_string(sock, Command("Down")) end, "Move focus down")
add_binding("n", alt.."k", Command("Up"), "Move focus up")
add_binding("n", alt.."l", Command("Right"), "Move focus right")

-- Alt + HJKL
add_binding("n", alt.."H", Command("SwapLeft"), "Swap pane left")
add_binding("n", alt.."J", Command("SwapDown"), "Swap pane down")
add_binding("n", alt.."K", Command("SwapUp"), "Swap pane up")
add_binding("n", alt.."L", Command("SwapRight"), "Swap pane right")

-- Misc.
add_binding("n", "\x0a", Event("\x0d"), nil) -- Convert newline to carriage return
add_binding("n", alt.."n", Command("Next"), "Next window")
add_binding("n", alt.."p", Command("Prev"), "Previous window")
add_binding("n", alt.."z", Command("Zoom"), "Zoom window")

-- Directional
add_binding("c", "\x1b[A", Command("Up"), "Move focus up")
add_binding("c", "\x1b[B", Command("Down"), "Move focus down")
add_binding("c", "\x1b[C", Command("Right"), "Move focus right")
add_binding("c", "\x1b[D", Command("Left"), "Move focus left")

-- Splits
-- TODO: There is currently no distinction between horizontal and vertical splits
add_binding("c", "|", Command("Split"), "Split horizontally")
add_binding("c", "_", Command("VSplit"), "Split vertically")
add_binding("c", "\"", Command("Split"), "Split horizontally")
add_binding("c", "%", Command("VSplit"), "Split vertically")

-- Layout
add_binding("c", "1", Command("Layout layout_even_horizontal"), "Even horizontal layout")
add_binding("c", "2", Command("Layout layout_even_vertical"), "Even vertical layout")
add_binding("c", "3", Command("Layout layout_main_horizontal"), "Main horizontal layout")
add_binding("c", "4", Command("Layout layout_main_vertical"), "Main vertical layout")
add_binding("c", "5", Command("Layout layout_tiled"), "Tiled layout")
add_binding("c", "6", Command("Layout layout_custom"), "Custom layout")

-- Directional
add_binding("c", "h", Command("Left"), "Move focus left")
add_binding("c", "j", Command("Down"), "Move focus down")
add_binding("c", "k", Command("Up"), "Move focus up")
add_binding("c", "l", Command("Right"), "Move focus right")

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

-- TODO: Page Up and Page Down

util.print_unused_bindings(bindings)
util.list_docs(bindings)
