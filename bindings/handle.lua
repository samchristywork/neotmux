function handle_binding_normal(sock, buf)
  for key, value in pairs(bindings["normal"]) do
    if (buf == key) then
      if (type(value) == "function") then
        value(sock)
      else
        write_string(sock, value)
      end
      return true
    end
  end
  return false
end

function handle_binding_control(sock, buf)
  for key, value in pairs(bindings["control"]) do
    if (buf == key) then
      if (type(value) == "function") then
        value(sock)
      else
        write_string(sock, value)
      end
      return true
    end
  end
  return false
end
