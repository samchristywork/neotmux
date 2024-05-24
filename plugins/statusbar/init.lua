require("bars")

bars = {
  {statusbar_left, default_bar},
  {statusbar_left, frame_bar},
}

function ntmux.statusbar(idx, cols, sessionName, windowTitles, mode)
  local ctrl = "\x1b[0m" -- Reset colors
  ctrl = ctrl .. "\x1b[7m" -- Enable reverse
  ctrl = ctrl .. "\x1b[38;5;4m" -- Set foreground color
  ctrl = ctrl .. "\x1b[1;1H" -- Move cursor to top left

  local idx = idx % #bars + 1
  local left, lWidth = bars[idx][1](sessionName, windowTitles, mode)
  local right, rWidth = bars[idx][2](sessionName, windowTitles, mode)
  padding = string.rep(" ", cols - lWidth - rWidth)

  return ctrl .. left .. padding .. right
end
