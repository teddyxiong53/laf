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

