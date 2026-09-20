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

-- Matches the aliases in data/scripts/XML/Scenes.XML - activateScene() loads a scene on
-- demand if it isn't already (see EC_SceneManager::activateScene), so no separate
-- loadScene() call is needed here.
sceneCycleNames = { "voxelchunkdemo", "shadowdebug", "yard", "physics_demo", "main", "streamed" }
sceneCycleIndex = 1 -- whichever scene is precached/active at startup is index 1

JUMP_STRENGTH = 2.6 -- gameplay decision, not an engine concern - see ArcadeCollision.lua's
                     -- own comment for the landing half of this mechanic. Tuned to an average
                     -- human standing vertical jump (~0.35m) at this engine's 9.8 gravity:
                     -- v = sqrt(2 * g * h).

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
    if key == "Space" then
        -- Straight vertical push, preserving whatever horizontal velocity already exists -
        -- a directional jump (factoring in currently-held movement keys) is the other
        -- option and is purely a gameplay choice, not something the engine mechanism cares
        -- about either way. Only fires if not already airborne (getIgnoreGravity() true ==
        -- grounded, per ArcadeCollision.lua's own mechanic).
        if entity:getIgnoreGravity() then
            local v = entity:getVelocity()
            entity:setVelocity(v.x, JUMP_STRENGTH, v.z)
            entity:setIgnoreGravity(false)
        end
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
    if key == "N" then
        sceneCycleIndex = (sceneCycleIndex % #sceneCycleNames) + 1
        game:activateScene(sceneCycleNames[sceneCycleIndex])
        print("CycleScenes: active scene " .. sceneCycleNames[sceneCycleIndex])
    end
    if key == "T" then
        -- Re-runs TerrainGeneration.lua and regenerates every voxel chunk live - see
        -- EC_VoxelChunkSystem::requestRegenerate(). Only requests it (flag checked once
        -- per frame on the main thread), so chunks visibly update over the next few
        -- frames, not instantly.
        game:regenerateTerrain()
        print("RegenerateTerrain: requested")
    end
end
