local cf = require "cf"
local sys = require "sys"

local f1 = function ()
    print('f1 at', sys.time())
    cf.sleep(1)
end
local f2 = function ()
    print('f2 at', sys.time())
    cf.sleep(2)
end

local f3 = function ()
    print('f3 at', sys.time())
    cf.sleep(3)
end


cf.join(f1, f2, f3)
print("end of code")