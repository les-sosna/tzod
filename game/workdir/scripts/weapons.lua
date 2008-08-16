-- weapons description
-- written by Insert
-------------------------------------------------------------------------------

-- helper functions
local function adjust_health(veh, h)
  veh.health = veh.health * (h / classes.default.health)
end

-------------------------------------------------------------------------------

function gc.weap_rockets.attach(veh)
  adjust_health(veh, 85)
  veh.mass = veh.mass * 1.16
  veh.inertia = veh.inertia * 1.2
end

-------------------------------------------------------------------------------

function gc.weap_autocannon.attach(veh)
--  message("Установлено орудие: автоматическая пушка")
  adjust_health(veh, 80)
end

-------------------------------------------------------------------------------

function gc.weap_cannon.attach(veh)
--  message("Установлено орудие: тяжелая пушка")
  adjust_health(veh, 125)
end

-------------------------------------------------------------------------------

function gc.weap_plazma.attach(veh)
--  message("Установлено орудие: плазменная пушка")
  adjust_health(veh, 100)
end

-------------------------------------------------------------------------------

function gc.weap_gauss.attach(veh)
  adjust_health(veh, 70)
end

-------------------------------------------------------------------------------

function gc.weap_ram.attach(veh)
  adjust_health(veh, 350)
  veh.mass = veh.mass * 2
  veh.power[1] = veh.power[1] * 2
  veh.percussion  = veh.percussion * 5
  veh.fragility   = veh.fragility * 0.5
end

-------------------------------------------------------------------------------

function gc.weap_bfg.attach(veh)
  adjust_health(veh, 110)
end

-------------------------------------------------------------------------------

function gc.weap_ripper.attach(veh)
  adjust_health(veh, 80)
end

-------------------------------------------------------------------------------

function gc.weap_minigun.attach(veh)
  veh.mass = veh.mass * 0.7
  adjust_health(veh, 65)
end

function gc.weap_zippo.attach(veh)
  adjust_health(veh, 130)
end

-------------------------------------------------------------------------------
-- end of file
