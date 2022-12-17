local sys = require "sys"
local time = sys.time

local co = require "internal.Co"
local ti = require "timer"

local ti_new = ti.new
local ti_start = ti.start

local co_new = co.new
local co_wait = co.wait
local co_spawn = co.spawn
local co_wakeup = co.wakeup
local co_self = co.self

local co_start = coroutine.resume
local co_wait_ex = coroutine.yield

local Timer = {}

local TMap = {}

local tab = debug.getregistry()
tab['__G_TIMER__'] = TMap

local function get_tid(offset)
    local ts = time()
    if offset > 0 then
        ts = ts + offset * 1e3
    end
    return ts*01.//1
end

local function get_ctx(tid)
    return TMap[tid]
end

local function set_ctx(tid, ctx)
    if not ctx then
        TMap[tid] = nil
        return
    end
    local list = TMap[tid]
    if not list then
        TMap[tid] = {ctx}
    else
        list[#list+1] = ctx
    end
    return ctx
end

local TTimer = ti_new()
Timer.TTimer = TTimer

Timer.TCo = co_new(function ()
    local run_idx = 1
    local time_idx = 2
    local func_idx = 3
    local again_idx = 4
    local async_idx = 5

    local TNow = get_tid(0)
    co_wait_ex(co_self())
    while true do
        local now = get_tid(0)
        for tid = TNow, now, 1 do
            local list = get_ctx(tid)
            if list then
                for idx = 1, #list do
                    local ctx = list[idx]
                    if ctx[run_idx] then
                        if ctx[async_idx] then
                            ctx[func_idx]()
                        else
                            co_spawn(ctx[func_idx])
                        end
                        if ctx[again_idx] then
                            set_ctx(get_tid(ctx[time_idx]), ctx)
                        end
                    end
                end
                -- 处理完了，置位空
                set_ctx(tid, nil)
            end
        end
        TNow = now + 1
        co_wait_ex()
    end
end)

co_start(Timer.TCo)
-- 0.01s的间隔启动
ti_start(TTimer, 0.01, Timer.TCo)

local class = require "class"

local TIMER = class("Timer")

function TIMER:ctor()
    -- do nothing
end

function TIMER:stop()
    if self then
        self[1] = false
    end

end

local function Timer_Init(timeout, again, async, func)
    return set_ctx(get_tid(timeout), setmetatable({true, timeout, func, again, async}, TIMER))
end

-- 一次性定时器
function Timer.timeout(timeout, callback)
    if type(timeout) ~= 'number' or timeout < 0 or type(callback) ~= 'function' then
        return
    end
    return Timer_Init(timeout, false, false, callback)
end

-- 重复定时器
function Timer.at(repeats, callback)
    if type(repeats) ~= 'number' or repeats <= 0 or type(callback) ~= 'function' then
        return
    end
    return Timer_Init(repeats, true, false, callback)
end

-- 休眠当前协程
function Timer.sleep(nsleep)
    if type(nsleep) ~= 'number' or nsleep < 0 then
        return
    end
    local coctx = co_self()
    Timer_Init(nsleep, false, true, function ()
        return co_wakeup(coctx)
    end)
    co_wait()
end

---comment 刷新
function Timer.flush()
  local Map = {}
  for key, value in pairs(TMap) do
    Map[key] = value
  end
  TMap = Map
  tab['__G_TIMER__'] = Map
end

return Timer