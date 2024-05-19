local bindings = {
  ["control"] = {},
  ["normal"] = {}
}

function print_unused_bindings(bindings)
  print("No control bindings for:")
  for i = 0, 25 do
    if bindings["control"][string.char(i + 97)] == nil then
      io.write(string.char(i + 97))
    end
  end

  print()
  print()

  print("No normal bindings for:")
  for i = 0, 25 do
    if bindings["normal"][string.char(i + 97)] == nil then
      io.write(string.char(i + 97))
    end
  end

  print()
end

function handle_binding_normal(sock, buf)
  for key, value in pairs(bindings["normal"]) do
    if (buf == key) then
      write_string(sock, value)
      return true
    end
  end
  return false
end

function handle_binding_control(sock, buf)
  for key, value in pairs(bindings["control"]) do
    if (buf == key) then
      write_string(sock, value)
      return true
    end
  end
  return false
end

-- Alt + hjkl
bindings["normal"]["\x1bh"] = "cLeft"
bindings["normal"]["\x1bj"] = "cDown"
bindings["normal"]["\x1bk"] = "cUp"
bindings["normal"]["\x1bl"] = "cRight"

-- Alt + HJKL
bindings["normal"]["\x1bH"] = "cSwapLeft"
bindings["normal"]["\x1bJ"] = "cSwapDown"
bindings["normal"]["\x1bK"] = "cSwapUp"
bindings["normal"]["\x1bL"] = "cSwapRight"

-- Misc.
bindings["normal"]["\x1bn"] = "cNext"
bindings["normal"]["\x1bp"] = "cPrev"
bindings["normal"]["\x1bz"] = "cZoom"

-- Directional
bindings["control"]["\x1b[A"] = "cUp"
bindings["control"]["\x1b[B"] = "cDown"
bindings["control"]["\x1b[C"] = "cRight"
bindings["control"]["\x1b[D"] = "cLeft"

-- Splits
bindings["control"]["|"] = "cSplit"
bindings["control"]["_"] = "cVSplit"
bindings["control"]["\""] = "cSplit"
bindings["control"]["%"] = "cVSplit"

-- Layout
bindings["control"]["1"] = "cEven_Horizontal"
bindings["control"]["2"] = "cEven_Vertical"
bindings["control"]["3"] = "cMain_Horizontal"
bindings["control"]["4"] = "cMain_Vertical"
bindings["control"]["5"] = "cTiled"
bindings["control"]["6"] = "cCustom"

-- Directional
bindings["control"]["h"] = "cLeft"
bindings["control"]["j"] = "cDown"
bindings["control"]["k"] = "cUp"
bindings["control"]["l"] = "cRight"

-- Window
bindings["control"]["e"] = "cCreate"
bindings["control"]["n"] = "cNext"
bindings["control"]["p"] = "cPrev"

-- Misc.
bindings["control"]["\x01"] = "e\x01" -- Ctrl + A
bindings["control"]["\x02"] = "els\n"
bindings["control"]["i"] = "cList"
bindings["control"]["q"] = "cQuit"
bindings["control"]["r"] = "cReloadLua"
bindings["control"]["x"] = "cCopySelection"
bindings["control"]["y"] = "cCycleStatus"
bindings["control"]["z"] = "cZoom"

-- TODO: Page Up and Page Down

print_unused_bindings(bindings)
