local timer = require "timer"
local sys = require "sys"

local t1 = timer.new()
local co = coroutine.create(function ()
    print("time after timeout 2s:", sys.timer())
end)
print("current time:", sys.time())
timer.start(t1, 2.0, co)

