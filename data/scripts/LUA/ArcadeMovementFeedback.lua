-- Footsteps, background music, and the on-screen debug readout for an entity using the
-- "arcade physics" pattern (see ArcadeCollision.lua/DebugSceneKeyDown.lua for the actual
-- ground-contact/jump mechanic). None of this is gravity or collision logic any more - it's
-- purely reacting to state the engine and the other two scripts already maintain
-- (RigidBody::ignoreGravity as the "am I grounded" signal, EC_DOD_Spatial::position/
-- velocity), so this only needs to run once per frame and look, not compute anything
-- physics-related itself.

local states = {}

local STEP_DISTANCE = 2.2 -- world units of horizontal travel between footstep sounds -
                           -- tuned by ear against the capsule's OnKeyHeld move speed (5.0/s)
                           -- to sound like a walking cadence, not a machine-gun of steps

-- Audio test hooks (Issue #112) - this demo scene doubles as the manual verification
-- vehicle for the new audio subsystem, since it already has a controllable capsule and no
-- other script is a better fit.
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

    -- Horizontal distance actually travelled this frame (not raw input) so bumping into a
    -- wall doesn't keep triggering steps, and only while grounded so falling/jumping
    -- doesn't. lastX/lastZ start nil (position not yet known on the very first frame)
    -- rather than 0 so that frame can't be mistaken for a real, possibly huge, step.
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
