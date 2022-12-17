local task = require "task"
local task_new = task.new
local task_start = task.start
local sys = require "sys"
local new_tab = sys.new_tab

local coroutine = coroutine
local co_new = coroutine.create
local co_start = coroutine.resume
local co_wait = coroutine.yield
local co_status = coroutine.status
local co_self = coroutine.running
local co_close = coroutine.close

local os_date = os.date
local fmt = string.format
local dbg_traceback = debug.traceback

local tunpack = table.unpack

local main_co = nil
local main_task = nil
local main_waited = true

local co_num = 0

local co_map = new_tab(0, 128)
co_map[co_self()] = {co_self(), nil, false}

local tab = debug.getregistry()
tab['__G_CO__'] = co_map

local co_wlist = new_tab(128, 0)

local function co_wrapper()
    return co_new(function ()
        -- 使用数字索引比hash索引更快
        local CO_INDEX, ARGS_INDEX, WAKEUP_INDEX = 1,2,3
        -- 使用数字下标比ipairs更快
        local start, total = 1, #co_wlist
        -- 使用两个fifo队列交替管理协程的运行与切换，
        -- 并且每次预分配的fifo队列大小跟上次执行的协程的数量有关。
        local co_rlist = co_wlist
        co_wlist = new_tab(32, 0)
        while true do
            for index = start, total do
                local obj = co_rlist[index]
                local co, args = obj[CO_INDEX], obj[ARGS_INDEX]
                local ok, errinfo
                if args then
                    ok, errinfo = co_start(co, tunpack(args)) -- 带参数的协程
                else
                    ok, errinfo = co_start(co) -- fork的协程不需要参数
                end
                -- 如果协程出错或者执行完毕，则去掉引用销毁
                if not ok or co_status(co) ~= 'suspended' then
                    if not ok then
                        -- 这个是出错的情况
                        -- 打印一下出错信息即可
                        print(fmt("[%s] [coroutine error]", os_date("%Y/%m%d %H:%M:%S"), dbg_traceback(co, errinfo, 1)))
                    end
                    -- 这个是完成的情况
                    -- 如果支持协程的销毁，那么销毁。
                    if co_close then
                        co_close(co)
                    end
                    -- 清空
                    co_map[co] = nil
                    co_num = co_num - 1
                end
                -- 这里是执行完的情况
                obj[ARGS_INDEX], obj[WAKEUP_INDEX] = nil, false
            end
            -- 如果没有执行对象，那么应该放弃执行权
            -- 等待有任务之后再次唤醒后再执行
            total = #co_wlist
            print("total ", total)
            if total == 0 then
                main_waited = true
                co_wait()
                total = #co_wlist
            end
            co_rlist = co_wlist
            co_wlist = new_tab(total>=128 and 128 or total, 0)
        end
    end)
end

local function co_check_init()
    -- 如果资源还没有初始化，先初始化
    if not main_task and not main_co then
        main_task = task_new()
        main_co = co_wrapper()
    end
    -- 如果协程没有启动，则启动协程开始运行
    if main_waited then
        main_waited = false
        task_start(main_task, main_co)
    end
end

local function co_add_queue(co, ...)
    local args = nil
    if select("#", ...) > 0 then
        args = {...}
    end
    local ctx = co_map[co]
    if not ctx then
        ctx = {co, args, true}
        co_map[co] = ctx
    else
        ctx[2], ctx[3] = args, true
    end
    co_wlist[#co_wlist+1] = ctx
end

local Co = {}

function Co.new(f)
    return co_new(f)
end

function Co.self()
    return co_self()
end

function Co.wait()
    assert(co_map[co_self()], "[coroutine error]: not associated internally")
    return co_wait()
end

function  Co.wakeup(co, ...)
    if type(co) ~= 'thread' then
        error("[coroutine error]: invalid coroutine")
    end
    if co == co_self() then
        error("[coroutine error]: can not wakeup a running co")
    end
    if co_status(co) ~= 'suspended' then
        error("[coroutine error]: invali co status [" .. co_status(co) .. "]")
    end
    local ctx = co_map[co]
    if not ctx then
        error("[coroutine error]: not associated internally")
    end
    if ctx[3] then
        error("[coroutine error]: try to wakeup a co multiple times")
    end
    co_check_init()
    co_add_queue(co, ...)
end

-- 创建协程
function Co.spawn(func, ...)
    assert(type(func)=='function', "[coroutine error]: invalid callback")
    co_num = co_num + 1
    local co = co_new(func)
    co_check_init()
    co_add_queue(co, ...)
    return co
end

function Co.count()
    return co_num
end

-- 刷新缓存
function Co.flush()
    local map = {}
    for key, value in pairs(co_map) do
        map[key] = value
    end
    co_map = map
    tab['__G_CO__'] = map
end

return Co