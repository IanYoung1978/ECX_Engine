-- Footsteps, background music and the on-screen debug readout for the arcade-physics entity. Reads
-- state only (ignoreGravity as "grounded", Spatial position/velocity); no physics happens here.

local states = {}

local STEP_DISTANCE = 2.2 -- world units between footstep sounds (a walking cadence at 5 m/s)

-- Background music starts once, on the first update.
local musicStarted = false

local debugTextID = nil
local debugTextReady = false

local function getState(entityId)
    local s = states[entityId]
    if not s then
        s = { distanceSinceStep = 0.0, lastX = nil, lastZ = nil }
        states[entityId] = s
    end
    return s
end

function update(entity, deltaTime)
    local entityId = entity:getID()
    local state = getState(entityId)
    local pos = entity:getPosition()
    local grounded = entity:getIgnoreGravity()

    if not musicStarted then
        musicStarted = true
        game:playMusic("data/assets/Sounds/mystic_theme.mp3", 0.5, true)
    end

    -- Footsteps use the horizontal distance actually travelled (not raw input), only while grounded;
    -- lastX/lastZ start nil so the first frame isn't read as a huge step.
    if state.lastX ~= nil then
        local dx = pos.x - state.lastX
        local dz = pos.z - state.lastZ
        if grounded then
            state.distanceSinceStep = state.distanceSinceStep + math.sqrt(dx * dx + dz * dz)
            if state.distanceSinceStep >= STEP_DISTANCE then
                state.distanceSinceStep = state.distanceSinceStep - STEP_DISTANCE
                game:playSound("data/assets/Sounds/stepdirt_1.wav", 0.6, "sfx")
            end
        else
            state.distanceSinceStep = 0.0
        end
    end
    state.lastX = pos.x
    state.lastZ = pos.z

    if not debugTextReady then
        local textEntity = game:getEntityByName("gravity_debug_text")
        local id = textEntity:getID()
        if id ~= 0 then
            debugTextID = id
            debugTextReady = true
        end
    end

    if debugTextReady then
        local v = entity:getVelocity()
        game:setUIText(debugTextID, string.format(
            "entity=%d pos=(%.2f, %.2f, %.2f) velocity=(%.2f, %.2f, %.2f) grounded=%s",
            entityId, pos.x, pos.y, pos.z, v.x, v.y, v.z, tostring(grounded)))
    end
end
