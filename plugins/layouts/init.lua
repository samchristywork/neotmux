function layout_even_horizontal(idx, count, width, height)
  if (count == 1) then
    return 0, 0, width, height
  end

  local beginCol = math.floor(idx * width / count)
  local endCol = math.floor((idx + 1) * width / count) - 1

  if (idx == count - 1) then
    return beginCol, 0, width - beginCol, height
  end

  return beginCol, 0, endCol - beginCol, height
end

function layout_even_vertical(idx, count, width, height)
  if (count == 1) then
    return 0, 0, width, height
  end

  local beginRow = math.floor(idx * height / count)
  local endRow = math.floor((idx + 1) * height / count) - 1

  if (idx == count - 1) then
    return 0, beginRow, width, height - beginRow
  end

  return 0, beginRow, width, endRow - beginRow
end

function layout_main_horizontal(idx, count, width, height)
  if (count == 1) then
    return 0, 0, width, height
  end

  local mainHeight = math.floor(height  / 2)

  if (idx == 0) then
    return 0, 0, width, mainHeight
  end

  local bottomHeight = height - mainHeight - 1

  local beginCol = math.floor((idx - 1) * width / (count - 1))
  local endCol = math.floor((idx) * width / (count - 1)) - 1

  if (idx == count - 1) then
    return beginCol, mainHeight + 1, width - beginCol, bottomHeight
  end

  return beginCol, mainHeight + 1, endCol - beginCol, bottomHeight
end

function layout_main_vertical(idx, count, width, height)
  if (count == 1) then
    return 0, 0, width, height
  end

  local mainWidth = math.floor(width / 2)

  if (idx == 0) then
    return 0, 0, mainWidth, height
  end

  local rightWidth = width - mainWidth - 1

  local beginRow = math.floor((idx - 1) * height / (count - 1))
  local endRow = math.floor((idx) * height / (count - 1)) - 1

  if (idx == count - 1) then
    return mainWidth + 1, beginRow, rightWidth, height - beginRow
  end

  return mainWidth + 1, beginRow, rightWidth, endRow - beginRow
end

function layout_custom(idx, count, width, height)
  if (idx == 0) then
    return width - 40, height - 10, 40, 10
  end

  local row = 4 * idx
  local col = 8 * idx
  return col, row, 40, 10
end
