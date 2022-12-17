local Timer = require "internal.Timer"
Timer.timeout(2.0, function ()
    print('2s timeout')
end)
Timer.at(3, function ()
    print('3s repeat')
end)

print("end of code")