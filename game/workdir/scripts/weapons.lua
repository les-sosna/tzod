-- weapons description
-- written by Insert
-------------------------------------------------------------------------------

-- helper functions
local function adjust_health(veh, h)
  veh.health = veh.health * (h / classes.default.health)
end

-- идея такая. при подборе оружия вызывается соответствующая функция, которая
-- может менять любые параметры класса.

-------------------------------------------------------------------------------

function gc.weap_rockets.attach(veh)
--  message("Установлено орудие: ракетница")
  adjust_health(veh, 85)
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
--  message("Установлено орудие: пушка Гаусса")
  adjust_health(veh, 70)
end

-------------------------------------------------------------------------------

function gc.weap_ram.attach(veh)
--  message("Установлено орудие: таран")
--  veh.mass = veh.mass * 2                -- тяжелый, сцуко =)
  adjust_health(veh, 350)                -- и жирный
  veh.percussion  = veh.percussion * 8
end

-------------------------------------------------------------------------------

function gc.weap_bfg.attach(veh)
--  message("Установлено орудие: БФГ");
  adjust_health(veh, 110)
end

-------------------------------------------------------------------------------

function gc.weap_ripper.attach(veh)
--  message("Установлено орудие: рипер")
  adjust_health(veh, 80)
end

-------------------------------------------------------------------------------

function gc.weap_minigun.attach(veh)
--  message("Установлено орудие: пулемет")
  veh.mass = veh.mass * 0.8
  adjust_health(veh, 65)
end

-------------------------------------------------------------------------------
-- end of file
