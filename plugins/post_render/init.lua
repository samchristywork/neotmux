function set_position(row, col)
  row = math.floor(row)
  col = math.floor(col)
  return "\x1b["..row..";"..col.."H"
end

function draw_rect(x, y, w, h, s)
  local ret = ""
  for i = 0, h-1 do
    ret = ret..set_position(y+i, x)
    for j = 0, w-1 do
      ret = ret..s
    end
  end
  return ret
end

function post_render(width, height)
  local w = 20
  local h = 10
  local x = width/2 - w/2
  local y = height/2 - h/2
  local ret = ""
  ret = ret .. draw_rect(x, y, w, h, "X")
  ret = ret .. draw_rect(x+1, y+1, w-2, h-2, " ")
  --return ret
  return ''
end
