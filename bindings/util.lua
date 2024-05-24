local util = {}

function util.print_unused_bindings()
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

function list_docs_mode(mode)
  local keys = {}
  for key, value in pairs(bindings[mode]) do
    table.insert(keys, key)
  end
  table.sort(keys)
  for _, key in ipairs(keys) do
    local value = bindings[mode][key]
    if value.docs ~= nil and value.docs ~= "" then
      local keyString = key:gsub("\x1b", "Alt+")
      print("", keyString, value.docs)
    end
  end
end

function util.list_docs()
  print("Control bindings:")
  print("", "Key", "Binding")
  list_docs_mode("control")
  print()
  print("Normal bindings:")
  print("", "Key", "Binding")
  list_docs_mode("normal")
end

return util
