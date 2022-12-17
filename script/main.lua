print("hello laf")
print("hello 123")
local class = require "class"

local TestClass = class("TestClass")

function TestClass:ctor(opt)
    self.name = opt.name
    print("test ctor")
end
function TestClass:print_name()
    print(self.name)
end

local test = TestClass:new({name='xxx'})
test:print_name()
print(_G)
local sys = require "sys"

print(sys)

print(sys.os())

-- local task = require "task"
-- local t1 = task.new()
-- task.start(t1, "a")

print("hello abc")