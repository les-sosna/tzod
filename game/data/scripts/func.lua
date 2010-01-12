-- func.lua
-- defines some usefull functions

local upval_dofile = dofile
function dofile(filename)
	upval_dofile("data/" .. filename)
end

local upval_loadfile = loadfile
function loadfile(filename)
	upval_loadfile("data/" .. filename)
end


function msgbox(handler_func, text, option1, option2, option3)
 print "msgbox function is deprecated; use service \"msgbox\" instead"
 if exists "__msgbox_deprecated_workaround_service" then 
  kill "__msgbox_deprecated_workaround_service"
 end
 user.__msgbox_deprecated_workaround_handler = handler_func
 service("msgbox", {
  name="__msgbox_deprecated_workaround_service",
  text=text,
  on_select=[[
    user.__msgbox_deprecated_workaround_handler(n)
    user.__msgbox_deprecated_workaround_handler = nil
  ]],
  option1=option1 or "OK",
  option2=option2,
  option3=option3,
 })
end


-- makes a deep copy of a given table (the 2nd param is optional and for internal use)
-- circular dependencies are correctly copied.
function tcopy(t, lookup_table)
 local copy = {}
 for i,v in pairs(t) do
  if type(v) ~= "table" then
   copy[i] = v
  else
   lookup_table = lookup_table or {}
   lookup_table[t] = copy
   if lookup_table[v] then
    copy[i] = lookup_table[v] -- we already copied this table. reuse the copy.
   else
    copy[i] = tcopy(v,lookup_table) -- not yet copied. copy it.
   end
  end
 end
 return copy
end


-- gets original class and modify it by weapon
function getvclass(cls, weap)
 if weap then
  local tmp = tcopy(classes[cls])
  gc[weap].attach(tmp) -- pass copy of vehicle class desc to the weapon
  return tmp           -- and return modified class desc
 end
 return classes[cls]
end


-- end of file
