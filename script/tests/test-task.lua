print("task code begin")
local task = require "task"
local t1 = task.new()
-- 下面这句是故意给错的，参数2应该是一个coroutine
-- task.start(t1, "a")
local co = coroutine.create(function ()
    print('hello task coroutine')
end)
task.start(t1, co)

print("task code end")