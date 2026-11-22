-- All OnKeyDown handling for this scene, bound once to the camera. Lua scripts share one global state
-- and are loaded once by filename, so two scripts each defining onKeyDown would clobber each other.

mouseCaptured = true
gamePaused = true

lightCycleNames = { "debug_sun", "debug_spot", "debug_point" }
lightCycleIndex = 0 -- 0 = nothing activated yet; first L press selects index 1

-- Aliases from Scenes.xml; activateScene() loads a scene on demand.
sceneCycleNames = { "voxelchunkdemo", "shadowdebug", "yard", "physics_demo", "main", "streamed" }
sceneCycleIndex = 1 -- whichever scene is precached/active at startup is index 1

JUMP_STRENGTH = 2.6 -- about a 0.35 m standing jump at gravity 9.8 (v = sqrt(2*g*h))

local function applyLightCycle()
    for i, name in ipairs(lightCycleNames) do
        -- activate()/deactivate() are no-ops on an invalid entity, so no lookup check is needed.
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
        -- Vertical push keeping any horizontal velocity; only when grounded (getIgnoreGravity() is true).
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
        -- Re-runs TerrainGeneration.lua and regenerates every chunk over the next few frames.
        game:regenerateTerrain()
        print("RegenerateTerrain: requested")
    end
end
