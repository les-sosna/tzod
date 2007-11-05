-- autocomplete.lua

local function split(src)
  local src_rev = string.reverse(src)
  local items = {}
  local tail
  local oldpos = 1
  repeat
    local res, pos = string.match(src_rev, "^%s*([%w_]*)%s*%.?()", oldpos)
    if not res or oldpos == pos then break end
    oldpos = pos
    if not tail then
      tail = string.reverse(res)
    else
      table.insert(items, 1, string.reverse(res))
    end
  until false
  tail = tail or ""
  return items, tail
end

local function _next(t,k)
  local m = getmetatable(t)
  local n = m and m.__next or next
  return n(t,k)
end

-- print all possible endings and return the common part
local function expand(path, tail, from)
  local result
  local prompt = {}
  local ending
  for k,v in _next, from do
    if 1 == string.find(k, tail, 1, false) then
      local m = getmetatable(v)
      if "table" == type(v) or (m and m.__next) then
        ending = "."
      elseif "function" == type(v) then
        ending = "()"
      else
        ending = " "
      end
      table.insert(prompt, k)
      if nil == result then
        result = k
      else
        local maxn = 0
        for n = 1,math.min(string.len(k), string.len(result)) do
          if string.sub(result, n, n) ~= string.sub(k, n, n) then
            break
          end
          maxn = n
        end
        result = string.sub(k, 1, maxn)
      end
    end
  end
  if 1 == #prompt then
    result = result .. ending
  elseif #prompt > 1 then
    print "--"
    table.sort(prompt)
    for k,v in ipairs(prompt) do
      print("? " .. path .. v)
    end
  end
  return result
end

function autocomplete(src)
  local items, tail = split(src)
  local current = _G
  local path = ""
  local expand_tail = true

  for _,v in ipairs(items) do
    local success, result = pcall(function(arg) return current[arg] end, v)
    if success then
      if nil ~= result then
        current = result
        path = path .. v .. "."
      else
        expand_tail = false
        for k,_ in pairs(current) do
          if 1 == string.find(k, v, 1, false) then
            print("?? " .. path .. k)
          end
        end
      end
    else
      print "field could not be indexed"
    --  error(result)
    end
  end

  local result
  if expand_tail and ("table" == type(current) or getmetatable(current).__next ) then
    result = expand(path, tail, current)
  end

  -- remove tail from the beginning
  if result then
    result = string.sub(result, string.len(tail) + 1)
  end

  return result
end


