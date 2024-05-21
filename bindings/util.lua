local util = {}

function util.print_unused_bindings(bindings)
  print("No control bindings for:")
  for i = 0, 25 do
    if bindings["control"][string.char(i + 97)] == nil then
      io.write(string.char(i + 97))
    end
  end

  print()
  print()

  print("No normal bindings for:")
  for i = 0, 25 do
    if bindings["normal"][string.char(i + 97)] == nil then
      io.write(string.char(i + 97))
    end
  end

  print()
end

function util.Command(cmd)
  return "c" .. cmd
end

function util.Event(evt)
  return "e" .. evt
end

return util
