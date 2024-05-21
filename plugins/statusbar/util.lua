local util = {}

function util.system(cmd)
  local f = io.popen(cmd)
  local l = f:read("*a")
  f:close()
  return l
end

return util
