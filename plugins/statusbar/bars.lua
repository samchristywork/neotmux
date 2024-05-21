local util = require("util")

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
  hostname = util.system("hostname"):gsub("\n", "")
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
