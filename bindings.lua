function handle_binding_normal(buf)
  -- Alt + hjkl
  if (buf == "\x1bh") then
    return "cLeft"
  elseif(buf == "\x1bj") then
    return "cDown"
  elseif(buf == "\x1bk") then
    return "cUp"
  elseif(buf == "\x1bl") then
    return "cRight"
  elseif(buf == "\x1bH") then
    return "cSwapLeft"
  elseif(buf == "\x1bJ") then
    return "cSwapDown"
  elseif(buf == "\x1bK") then
    return "cSwapUp"
  elseif(buf == "\x1bL") then
    return "cSwapRight"
  end

  if(buf == "\x1bn") then
    return "cNext"
  elseif(buf == "\x1bp") then
    return "cPrev"
  elseif (buf == "\x1bz") then
    return "cZoom"
  end

  return NULL
end

function handle_binding_control(buf)
  -- if (buf == "\x1b[5~") then
    -- return "cScrollUp" --elseif(buf == "\x1b[6~") then
    -- return "cScrollDown"
  if (buf == "\x1b[D") then
    return "cLeft"
  elseif(buf == "\x1b[C") then
    return "cRight"
  elseif(buf == "\x1b[A") then
    return "cUp"
  elseif(buf == "\x1b[B") then
    return "cDown"
  elseif(buf == "|") then
    return "cSplit"
  elseif(buf == "_") then
    return "cVSplit"
  elseif(buf == "\"") then
    return "cSplit"
  elseif(buf == "%") then
    return "cVSplit"
  elseif(buf == "i") then
    return "cList"
  elseif(buf == "y") then
    return "cCycleStatus"
  elseif(buf == "e") then
    return "cCreate"
  elseif(buf == "r") then
    return "cReloadLua"
  elseif(buf == "h") then
    return "cLeft"
  elseif(buf == "j") then
    return "cDown"
  elseif(buf == "k") then
    return "cUp"
  elseif(buf == "l") then
    return "cRight"
  elseif(buf == "z") then
    return "cZoom"
  elseif(buf == "1") then
    return "cEven_Horizontal"
  elseif(buf == "2") then
    return "cEven_Vertical"
  elseif(buf == "3") then
    return "cMain_Horizontal"
  elseif(buf == "4") then
    return "cMain_Vertical"
  elseif(buf == "5") then
    return "cTiled"
  elseif(buf == "6") then
    return "cCustom"
  elseif(buf == "n") then
    return "cNext"
  elseif(buf == "p") then
    return "cPrev"
  end

  return NULL
end
