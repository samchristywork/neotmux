function handle_binding_normal(sock, buf)
  for key, value in pairs(bindings.normal) do
    if (buf == key) then
      local command = value.command
      if (type(command) == "function") then
        command(sock)
      else
        write_string(sock, command)
      end
      return true
    end
  end
  return false
end

function handle_binding_control(sock, buf)
  for key, value in pairs(bindings.control) do
    if (buf == key) then
      local command = value.command
      if (type(command) == "function") then
        command(sock)
      else
        write_string(sock, command)
      end
      return true
    end
  end
  return false
end
