local bindings = {
  ["control"] = {},
  ["normal"] = {}
}

-- TODO: Should throw error on syntax error
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
bindings["control"]["\x1b[D"] = "cLeft"
bindings["control"]["\x1b[C"] = "cRight"
bindings["control"]["\x1b[A"] = "cUp"
bindings["control"]["\x1b[B"] = "cDown"

-- Splits
bindings["control"]["\x1b|"] = "cSplit"
bindings["control"]["\x1b_"] = "cVSplit"
bindings["control"]["\x1b\""] = "cSplit"
bindings["control"]["\x1b%"] = "cVSplit"

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
bindings["control"]["i"] = "cList"
bindings["control"]["y"] = "cCycleStatus"
bindings["control"]["r"] = "cReloadLua"
bindings["control"]["z"] = "cZoom"

-- TODO: Page Up and Page Down
