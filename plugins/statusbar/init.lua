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

  return left
end

frame = 0
numBars = 3
function statusbar_right(idx, numBars)
  frame = frame + 1
  idx = idx % numBars
  if idx == 0 then
    hostname = system("hostname")
    hostname = hostname:gsub("\n", "")
    date = os.date("%H:%M %d-%b-%Y")
    resetColor = "\x1b[38;5;4m"

    ret = resetColor .. ' "' .. hostname .. '" ' .. date
    return ret, #hostname + #date + 4
  elseif idx == 1 then
    ret = "" .. "Implement your own bar here"
    return ret, #ret
  else
    ret = "" .. frame
    return ret, #ret
  end
end

function statusbar(idx, cols, sessionName, windowTitles, mode)
  left = statusbar_left(sessionName, windowTitles, mode)
  right, width = statusbar_right(idx, numBars)
  padding = string.rep(" ", cols - #left - width)
  return left .. padding .. right
end
