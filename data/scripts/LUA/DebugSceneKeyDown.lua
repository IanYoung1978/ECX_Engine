-- All OnKeyDown handling for ShadowDebug.xml in one script, bound once to the camera
-- entity. NOT split across multiple scripts each defining their own onKeyDown(): every
-- Lua script in this project shares a single global interpreter state
-- (EC_LuaScriptSystem::m_luaState) and is executed exactly once via luaL_dofile, cached by
-- filename - two scripts both defining a global function named onKeyDown silently clobber
-- each other (whichever loads second wins, permanently), not layer/coexist. That's a real
-- gap in the scripting system worth fixing properly later; consolidating here avoids it for
-- now without touching the project's existing shared OnKeyDown.lua.

mouseCaptured = true
gamePaused = true

lightCycleNames = { "debug_sun", "debug_spot", "debug_point" }
lightCycleIndex = 0 -- 0 = nothing activated yet; first L press selects index 1

local function applyLightCycle()
    for i, name in ipairs(lightCycleNames) do
        -- activate()/deactivate() are no-ops on an invalid/dead entity ID, so no need to
        -- check the lookup succeeded before calling them.
        local light = game:getEntityByName(name)
        if i == lightCycleIndex then
            light:activate()
        else
            light:deactivate()
        end
    end
end

function onKeyDown(entity, event)
    local key = event:getKey()

    if key == "Escape" then
        print("Escape key pressed, shutting down the game.")
        game:shutdown()
    end
    if key == "F3" then
        debugOverlayVisible = not debugOverlayVisible
    end
    if key == "F4" then
        mouseCaptured = not mouseCaptured
        game:setMouseCaptured(mouseCaptured)
    end
    if key == "P" then
        gamePaused = not gamePaused
        if gamePaused then
            game:pauseGame()
        else
            game:resumeGame()
        end
    end
    if key == "L" then
        lightCycleIndex = (lightCycleIndex % #lightCycleNames) + 1
        applyLightCycle()
        print("CycleLights: active light " .. lightCycleNames[lightCycleIndex])
    end
end
