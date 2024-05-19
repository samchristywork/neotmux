function system(cmd)
  local f = io.popen(cmd)
  local l = f:read("*a")
  f:close()
  return l
end

function statusbar_left(sessionName, windowTitles, mode)
  left = ""
  left = left .. mode
  left = left .. " [" .. sessionName .. "]"
  for i, e in ipairs(windowTitles) do
    left = left .. "  " .. e.title
    if e.active then
      left = left .. "*"
    end
    if e.zoom then
      left = left .. "Z"
    end
  end

  return left, #left
end


function default_bar(sessionName, windowTitles, mode)
  hostname = system("hostname"):gsub("\n", "")
  date = os.date("%H:%M %d-%b-%Y")
  ret = ' "' .. hostname .. '" ' .. date

  return ret, #ret
end

local frame = 0;
function frame_bar(sessionName, windowTitles, mode)
  ret = "" .. frame
  frame = frame + 1
  return ret, #ret
end

function statusbar(idx, cols, sessionName, windowTitles, mode)
  local bars = {
    {statusbar_left, default_bar},
    {statusbar_left, frame_bar},
  }

  local idx = idx % #bars + 1
  local left, lWidth = bars[idx][1](sessionName, windowTitles, mode)
  local right, rWidth = bars[idx][2](sessionName, windowTitles, mode)
  padding = string.rep(" ", cols - lWidth - rWidth)

  return left .. padding .. right
end
