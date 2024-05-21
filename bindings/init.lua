local util = require("bindings.util")
local Command = util.Command
local Event = util.Event

-- NOTE: Must be global.
bindings = {
  ["control"] = {},
  ["normal"] = {}
}

local alt = "\x1b"

local normal = bindings["normal"]
local control = bindings["control"]

-- Alt + hjkl
normal[alt.."h"] = Command("Left")
normal[alt.."j"] = function(sock) write_string(sock, Command("Down")) end
normal[alt.."k"] = Command("Up")
normal[alt.."l"] = Command("Right")

-- Alt + HJKL
normal[alt.."H"] = Command("SwapLeft")
normal[alt.."J"] = Command("SwapDown")
normal[alt.."K"] = Command("SwapUp")
normal[alt.."L"] = Command("SwapRight")

-- Misc.
normal["\x0a"] = Event("\x0d") -- Convert newline to carriage return
normal[alt.."?"] = Command("Help")
normal[alt.."n"] = Command("Next")
normal[alt.."p"] = Command("Prev")
normal[alt.."z"] = Command("Zoom")

-- Directional
control["\x1b[A"] = Command("Up")
control["\x1b[B"] = Command("Down")
control["\x1b[C"] = Command("Right")
control["\x1b[D"] = Command("Left")

-- Splits
-- TODO: There is currently no distinction between horizontal and vertical splits
control["|"] = Command("Split")
control["_"] = Command("VSplit")
control["\""] = Command("Split")
control["%"] = Command("VSplit")

-- Layout
control["1"] = Command("Layout layout_even_horizontal")
control["2"] = Command("Layout layout_even_vertical")
control["3"] = Command("Layout layout_main_horizontal")
control["4"] = Command("Layout layout_main_vertical")
control["5"] = Command("Layout layout_tiled")
control["6"] = Command("Layout layout_custom")

-- Directional
control["h"] = Command("Left")
control["j"] = Command("Down")
control["k"] = Command("Up")
control["l"] = Command("Right")

-- Window
control["e"] = Command("Create")
control["n"] = Command("Next")
control["p"] = Command("Prev")

-- Misc.
control["\x01"] = Event("\x01") -- Ctrl + A
control["\x02"] = Event("ls\n") -- Ctrl + B
control["i"] = Command("List")
control["q"] = Command("Quit")
control["r"] = Command("ReloadLua")
control["x"] = Command("CopySelection")
control["y"] = Command("CycleStatus")
control["z"] = Command("Zoom")

-- TODO: Page Up and Page Down

util.print_unused_bindings(bindings)
