--------------------------------------------------------------------------------
-- COMPREHENSIVE LUA SCRIPT EXAMPLES
-- Shows all available event handlers and common patterns
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- LIFECYCLE EVENTS
--------------------------------------------------------------------------------

function onEntityCreate(entity, event)
    print("Entity created: " .. entity:getName())
    -- Initialize script variables
    entity:setFloat("health", 100.0)
    entity:setString("state", "idle")
end

function onEntityLoaded(entity, event)
    print("Entity loaded from file: " .. entity:getName())
    -- Post-load initialization
end

function onEntityKill(entity, event)
    print("Entity killed: " .. entity:getName())
    -- Play death animation, spawn particles, etc.
end

function onEntityDestroy(entity, event)
    print("Entity being destroyed: " .. entity:getName())
    -- Final cleanup
end

--------------------------------------------------------------------------------
-- UPDATE LOOP
--------------------------------------------------------------------------------

function update(entity, deltaTime)
    -- Called every frame
    local health = entity:getFloat("health", 100.0)
    
    -- Example: Regenerate health over time
    if health < 100.0 then
        health = health + 5.0 * deltaTime
        entity:setFloat("health", math.min(health, 100.0))
    end
    
    -- Example: Simple state machine
    local state = entity:getString("state", "idle")
    if state == "moving" then
        -- Movement logic handled by onKeyHeld
    elseif state == "attacking" then
        local attackTime = entity:getFloat("attackTime", 0.0)
        attackTime = attackTime + deltaTime
        if attackTime >= 1.0 then
            entity:setString("state", "idle")
            entity:setFloat("attackTime", 0.0)
        else
            entity:setFloat("attackTime", attackTime)
        end
    end
end

--------------------------------------------------------------------------------
-- KEYBOARD INPUT
--------------------------------------------------------------------------------

function onKeyDown(entity, event)
    -- Called once when key is first pressed
    local key = event:getKey()
    
    if key == "Space" then
        -- Jump
        local vel = entity:getVelocity()
        vel.y = 10.0
        entity:setVelocity(vel.x, vel.y, vel.z)
        print("Jump!")
        
    elseif key == "E" then
        -- Interact
        print("Interact with object")
        
    elseif key == "Q" then
        -- Toggle state
        local state = entity:getString("state", "idle")
        if state == "idle" then
            entity:setString("state", "sneaking")
            print("Started sneaking")
        else
            entity:setString("state", "idle")
            print("Stopped sneaking")
        end
        
    elseif key == "F" then
        -- Attack
        entity:setString("state", "attacking")
        entity:setFloat("attackTime", 0.0)
        print("Attack!")
    end
end

function onKeyUp(entity, event)
    -- Called once when key is released
    local key = event:getKey()
    
    if key == "W" or key == "A" or key == "S" or key == "D" then
        -- Stop moving
        entity:setVelocity(0, entity:getVelocity().y, 0)
    end
end

function onKeyHeld(entity, event)
    -- Called continuously while key is held
    local key = event:getKey()
    local speed = 5.0
    local dt = 0.016  -- Approximate frame time
    
    -- Movement
    if key == "W" then
        entity:moveForward(speed * dt)
    elseif key == "S" then
        entity:moveBack(speed * dt)
    elseif key == "A" then
        entity:moveLeft(speed * dt)
    elseif key == "D" then
        entity:moveRight(speed * dt)
    end
    
    -- Camera/rotation
    if key == "Left" then
        local orient = entity:getOrientation()
        entity:setOrientation(orient.x, orient.y - 1.0 * dt, orient.z)
    elseif key == "Right" then
        local orient = entity:getOrientation()
        entity:setOrientation(orient.x, orient.y + 1.0 * dt, orient.z)
    elseif key == "Up" then
        local orient = entity:getOrientation()
        entity:setOrientation(orient.x + 1.0 * dt, orient.y, orient.z)
    elseif key == "Down" then
        local orient = entity:getOrientation()
        entity:setOrientation(orient.x - 1.0 * dt, orient.y, orient.z)
    end
end

--------------------------------------------------------------------------------
-- MOUSE INPUT
--------------------------------------------------------------------------------

function onMouseDown(entity, event)
    local button = event:getMouseButton()
    
    if button == 1 then  -- Left click
        print("Left mouse clicked")
        -- Example: Fire weapon
        entity:setString("state", "shooting")
        
    elseif button == 2 then  -- Right click
        print("Right mouse clicked")
        -- Example: Aim down sights
        entity:setFloat("aimZoom", 2.0)
    end
end

function onMouseUp(entity, event)
    local button = event:getMouseButton()
    
    if button == 2 then
        -- Stop aiming
        entity:setFloat("aimZoom", 1.0)
    end
end

function onMouseHeld(entity, event)
    local button = event:getMouseButton()
    
    if button == 1 then
        -- Continuous fire weapon
        local fireRate = entity:getFloat("fireRate", 0.1)
        local timeSinceLastShot = entity:getFloat("timeSinceLastShot", 0.0)
        
        if timeSinceLastShot >= fireRate then
            print("Fire!")
            entity:setFloat("timeSinceLastShot", 0.0)
        end
    end
end

function onMouseMove(entity, event)
    local mouseX = event:getMouseX()
    local mouseY = event:getMouseY()
    
    -- Mouse look (first-person camera)
    local sensitivity = 0.002
    local orient = entity:getOrientation()
    
    -- Yaw (left/right)
    orient.y = orient.y - mouseX * sensitivity
    
    -- Pitch (up/down) with clamping
    orient.x = orient.x - mouseY * sensitivity
    orient.x = math.max(-1.5, math.min(1.5, orient.x))  -- Clamp to ~85 degrees
    
    entity:setOrientation(orient.x, orient.y, orient.z)
end

--------------------------------------------------------------------------------
-- COLLISION EVENTS
--------------------------------------------------------------------------------

function onCollisionBegin(entity, event)
    local otherUID = event:getOtherEntityUID()
    print("Collision started with entity: " .. otherUID)
    
    -- Example: Take damage from hazard
    local health = entity:getFloat("health", 100.0)
    entity:setFloat("health", health - 10.0)
    
    if health <= 0 then
        print("Entity died!")
        entity:deactivate()
    end
end

function onCollisionEnd(entity, event)
    local otherUID = event:getOtherEntityUID()
    print("Collision ended with entity: " .. otherUID)
end

--------------------------------------------------------------------------------
-- ENTITY STATE CHANGE EVENTS
--------------------------------------------------------------------------------

function onPositionChanged(entity, event)
    local newPos = event:getNewPosition()
    print("Position changed to: " .. newPos.x .. ", " .. newPos.y .. ", " .. newPos.z)
    
    -- Example: Trigger zone check
    if newPos.y < -10.0 then
        print("Fell off the world!")
        entity:setPosition(0, 10, 0)  -- Respawn
    end
end

function onOrientationChanged(entity, event)
    local newOrient = event:getNewOrientation()
    -- React to orientation changes
end

function onVelocityChanged(entity, event)
    local newVel = event:getNewVelocity()
    
    -- Example: Play sound when moving fast
    local speed = math.sqrt(newVel.x * newVel.x + newVel.y * newVel.y + newVel.z * newVel.z)
    if speed > 20.0 then
        print("Moving very fast!")
    end
end

function onAngularVelocityChanged(entity, event)
    local newAngVel = event:getNewAngularVelocity()
    -- React to rotation speed changes
end

function onStopRotation(entity, event)
    print("Rotation stopped")
    entity:setAngularVelocity(0, 0, 0)
end

function onStopMotion(entity, event)
    print("Motion stopped")
    entity:setVelocity(0, 0, 0)
end

--------------------------------------------------------------------------------
-- SYSTEM EVENTS
--------------------------------------------------------------------------------

function onWorldLoaded(entity, event)
    print("World loaded - entity ready: " .. entity:getName())
    -- Initialize world-specific logic
end

function onConfigLoaded(entity, event)
    print("Config loaded")
    -- Load configuration-specific behavior
end

function onSystemUpdate(entity, event)
    -- Called when system needs update (rare)
end

--------------------------------------------------------------------------------
-- COMMON PATTERNS
--------------------------------------------------------------------------------

-- Pattern 1: Timer/Cooldown
function updateCooldowns(entity, deltaTime)
    local attackCooldown = entity:getFloat("attackCooldown", 0.0)
    if attackCooldown > 0 then
        entity:setFloat("attackCooldown", attackCooldown - deltaTime)
    end
end

-- Pattern 2: State Machine
function handleStateMachine(entity, deltaTime)
    local state = entity:getString("state", "idle")
    
    if state == "idle" then
        -- Idle logic
    elseif state == "moving" then
        -- Movement logic
    elseif state == "attacking" then
        -- Attack logic
    elseif state == "dead" then
        -- Death logic
    end
end

-- Pattern 3: AI Behavior
function simpleAI(entity, deltaTime)
    -- Find player
    local player = game:getEntity("Player")
    if player then
        local myPos = entity:getPosition()
        local playerPos = player:getPosition()
        
        -- Calculate direction to player
        local dx = playerPos.x - myPos.x
        local dz = playerPos.z - myPos.z
        local distance = math.sqrt(dx * dx + dz * dz)
        
        if distance < 10.0 then
            -- Chase player
            local speed = 2.0
            local vx = (dx / distance) * speed
            local vz = (dz / distance) * speed
            entity:setVelocity(vx, 0, vz)
        else
            -- Idle
            entity:setVelocity(0, 0, 0)
        end
    end
end

-- Pattern 4: Health System
function handleHealth(entity, deltaTime)
    local health = entity:getFloat("health", 100.0)
    local maxHealth = 100.0
    
    -- Regeneration
    if health < maxHealth then
        health = health + 5.0 * deltaTime
        entity:setFloat("health", math.min(health, maxHealth))
    end
    
    -- Death check
    if health <= 0 then
        entity:setString("state", "dead")
        entity:deactivate()
    end
end

--------------------------------------------------------------------------------
-- MINIMAL SCRIPT EXAMPLE
-- You only need to implement the functions you actually use!
--------------------------------------------------------------------------------

--[[
-- Example: Simple player controller (minimal)

function onKeyHeld(entity, event)
    local key = event:getKey()
    if key == "W" then entity:moveForward(0.1) end
    if key == "S" then entity:moveBack(0.1) end
    if key == "A" then entity:moveLeft(0.1) end
    if key == "D" then entity:moveRight(0.1) end
end

function onKeyDown(entity, event)
    if event:getKey() == "Space" then
        local vel = entity:getVelocity()
        entity:setVelocity(vel.x, 10.0, vel.z)  -- Jump
    end
end

-- That's it! Just 15 lines for a working player controller.
]]--