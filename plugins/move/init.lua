function is_within_rect(rect, pos)
  return pos.row >= rect.start_row and
    pos.row < rect.end_row and
    pos.col >= rect.start_col and
    pos.col < rect.end_col
end

function handleLeft(pane, panes, width, height)
  col = pane.col
  row = pane.row

  while true do
    col = col - 1

    if col < 0 then
      col = width - 1
    end

    for r = row, row + pane.height - 1 do
      pos = {col = col, row = r}
      for i, p in ipairs(panes) do
        rect = {
          start_col = p.col,
          start_row = p.row,
          end_col = p.col + p.width,
          end_row = p.row + p.height
        }
        if is_within_rect(rect, pos) then
          return i - 1
        end
      end
    end
  end

  return 0
end

function handleRight(pane, panes, width, height)
  col = pane.col
  row = pane.row

  col = col + pane.width

  while true do
    col = col + 1

    if col >= width then
      col = 0
    end

    for r = row, row + pane.height - 1 do
      pos = {col = col, row = r}
      for i, p in ipairs(panes) do
        rect = {
          start_col = p.col,
          start_row = p.row,
          end_col = p.col + p.width,
          end_row = p.row + p.height
        }
        if is_within_rect(rect, pos) then
          return i - 1
        end
      end
    end
  end

  return 0
end

function handleUp(pane, panes, width, height)
  col = pane.col
  row = pane.row

  while true do
    row = row - 1

    if row < 0 then
      row = height - 1
    end

    for c = col, col + pane.width - 1 do
      pos = {col = c, row = row}
      for i, p in ipairs(panes) do
        rect = {
          start_col = p.col,
          start_row = p.row,
          end_col = p.col + p.width,
          end_row = p.row + p.height
        }
        if is_within_rect(rect, pos) then
          return i - 1
        end
      end
    end
  end

  return 0
end

function handleDown(pane, panes, width, height)
  col = pane.col
  row = pane.row

  row = row + pane.height

  while true do
    row = row + 1

    if row >= height then
      row = 0
    end

    for c = col, col + pane.width - 1 do
      pos = {col = c, row = row}
      for i, p in ipairs(panes) do
        rect = {
          start_col = p.col,
          start_row = p.row,
          end_col = p.col + p.width,
          end_row = p.row + p.height
        }
        if is_within_rect(rect, pos) then
          return i - 1
        end
      end
    end
  end

  return 0
end
