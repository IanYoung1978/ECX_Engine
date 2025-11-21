# Event Handler Reference

## Complete Event-to-Function Mapping

| C++ Event Type | Lua Function Name | When Called | Common Use Cases |
|----------------|-------------------|-------------|------------------|
| `EntityCreate` | `onEntityCreate` | Entity first created | Initialization, spawn effects |
| `EntityKill` | `onEntityKill` | Entity killed (still exists) | Death animation, drop loot |
| `EntityDestroy` | `onEntityDestroy` | Before entity deleted | Final cleanup, save state |
| `entity_loaded` | `onEntityLoaded` | After entity loaded from file | Post-load setup |
| `EntityStopRotation` | `onStopRotation` | Rotation stops | Animation changes |
| `EntityStopMotion` | `onStopMotion` | Movement stops | Idle state, stop effects |
| `EntityChangePosition` | `onPositionChanged` | Position changes | Trigger zones, footsteps |
| `EntityChangeOrientation` | `onOrientationChanged` | Orientation changes | Look direction updates |
| `EntityChangeAngularVelocity` | `onAngularVelocityChanged` | Angular velocity changes | Rotation effects |
| `EntityChangeVelocity` | `onVelocityChanged` | Velocity changes | Speed-based effects |
| `CollisionBeginEvent` | `onCollisionBegin` | Collision starts | Damage, pickups, triggers |
| `CollisionEndEvent` | `onCollisionEnd` | Collision ends | Exit triggers |
| `key_down` | `onKeyDown` | Key pressed (once) | Jump, attack, interact |
| `key_up` | `onKeyUp` | Key released (once) | Stop actions |
| `key_held` | `onKeyHeld` | Key held (continuous) | Movement, aiming |
| `mouse_down` | `onMouseDown` | Mouse button pressed | Fire weapon, select |
| `mouse_up` | `onMouseUp` | Mouse button released | Stop firing |
| `mouse_held` | `onMouseHeld` | Mouse button held | Continuous fire |
| `mouse_move` | `onMouseMove` | Mouse moves | Camera control |
| `world_loaded` | `onWorldLoaded` | World finishes loading | Level-specific setup |
| `config_loaded` | `onConfigLoaded` | Config files loaded | Apply settings |
| `system_update` | `onSystemUpdate` | System requests update | Rare, special cases |
| (Every frame) | `update` | Every frame | Main logic loop |

---

## Event API Reference

### Input Events

```lua
-- Keyboard Events (onKeyDown, onKeyUp, onKeyHeld)
function onKeyDown(entity, event)
    local key = event:getKey()  -- Returns: "W", "Space", "Escape", etc.
end

-- Mouse Button Events (onMouseDown, onMouseUp, onMouseHeld)
function onMouseDown(entity, event)
    local button = event:getMouseButton()  -- Returns: 1 (left), 2 (right), 3 (middle)
end

-- Mouse Movement Events (onMouseMove)
function onMouseMove(entity, event)
    local dx = event:getMouseX()  -- Mouse movement in X
    local dy = event:getMouseY()  -- Mouse movement in Y
end
```

### Collision Events

```lua
function onCollisionBegin(entity, event)
    local otherUID = event:getOtherEntityUID()  -- Returns: Entity ID of other object
    
    -- Get the other entity (if you need more info)
    local other = game:getEntity("EntityName")  -- Or use entity manager
end

function onCollisionEnd(entity, event)
    local otherUID = event:getOtherEntityUID()
end
```

### Entity State Change Events

```lua
function onPositionChanged(entity, event)
    local newPos = event:getNewPosition()  -- Returns: vec3
    print("New position: " .. newPos.x .. ", " .. newPos.y .. ", " .. newPos.z)
end

function onOrientationChanged(entity, event)
    local newOrient = event:getNewOrientation()  -- Returns: vec3 (euler angles)
end

function onVelocityChanged(entity, event)
    local newVel = event:getNewVelocity()  -- Returns: vec3
end

function onAngularVelocityChanged(entity, event)
    local newAngVel = event:getNewAngularVelocity()  -- Returns: vec3
end
```

---

## Entity API Reference

### Basic Properties

```lua
entity:getName()              -- Get entity name
entity:getUID()               -- Get unique ID
entity:isActive()             -- Check if active
entity:activate()             -- Activate entity
entity:deactivate()           -- Deactivate entity
```

### Spatial Properties

```lua
-- Position
local pos = entity:getPosition()       -- Returns vec3
entity:setPosition(x, y, z)            -- Set position

-- Velocity
local vel = entity:getVelocity()       -- Returns vec3
entity:setVelocity(x, y, z)            -- Set velocity

-- Orientation (Euler angles)
local orient = entity:getOrientation() -- Returns vec3
entity:setOrientation(x, y, z)         -- Set orientation

-- Angular Velocity
local angVel = entity:getAngularVelocity()  -- Returns vec3
entity:setAngularVelocity(x, y, z)          -- Set angular velocity
```

### Direction Vectors

```lua
local forward = entity:getForward()    -- Returns vec3 (forward direction)
local up = entity:getUp()              -- Returns vec3 (up direction)
local right = entity:getRight()        -- Returns vec3 (right direction)
```

### Movement Helpers

```lua
entity:moveForward(amount)    -- Move in forward direction
entity:moveBack(amount)       -- Move backward
entity:moveLeft(amount)       -- Move left (strafe)
entity:moveRight(amount)      -- Move right (strafe)
entity:moveUp(amount)         -- Move up (world space)
entity:moveDown(amount)       -- Move down (world space)
entity:rotateAroundAxis(angle, x, y, z)  -- Rotate around axis
```

### Script Variables (Persistent State)

```lua
-- Float variables
entity:setFloat("health", 100.0)
local health = entity:getFloat("health", 100.0)  -- Default: 100.0

-- String variables
entity:setString("state", "idle")
local state = entity:getString("state", "idle")  -- Default: "idle"
```

---

## vec3 API

```lua
-- Create vector
local v = vec3(1.0, 2.0, 3.0)

-- Access components
local x = v.x
local y = v.y
local z = v.z

-- Modify components
v.x = 5.0
v.y = 10.0
v.z = 15.0
```

---

## game API Reference
```lua
-- Get entity by name
local entity = game:getEntity("EntityName")
-- shutdown the game
game:shutdown()
-- get key state
    local isPressed = game:isKeyPressed("W")  -- true/false

```
---


## Complete Example Scripts

### Example 1: Player Controller

```lua
-- scripts/player.lua

local moveSpeed = 5.0
local jumpForce = 10.0
local mouseSensitivity = 0.002

function onKeyHeld(entity, event)
    local key = event:getKey()
    local dt = 0.016  -- ~60fps
    
    if key == "W" then entity:moveForward(moveSpeed * dt) end
    if key == "S" then entity:moveBack(moveSpeed * dt) end
    if key == "A" then entity:moveLeft(moveSpeed * dt) end
    if key == "D" then entity:moveRight(moveSpeed * dt) end
end

function onKeyDown(entity, event)
    if event:getKey() == "Space" then
        local vel = entity:getVelocity()
        entity:setVelocity(vel.x, jumpForce, vel.z)
    end
end

function onMouseMove(entity, event)
    local mouseX = event:getMouseMotionX()
    local mouseY = event:getMouseMotionY()
    
    local sensitivity = 0.02
    
    local orient = entity:getOrientation()
    
    local newYaw = orient.y - mouseX * sensitivity
    local newPitch = orient.x - mouseY * sensitivity
    
    newPitch = math.max(-1.5, math.min(1.5, newPitch))
    
    entity:setOrientation(newPitch, newYaw, orient.z)
end
```

### Example 2: Health/Damage System

```lua
-- scripts/damageable.lua

function onEntityCreate(entity, event)
    entity:setFloat("health", 100.0)
    entity:setFloat("maxHealth", 100.0)
end

function update(entity, deltaTime)
    -- Regenerate health
    local health = entity:getFloat("health")
    local maxHealth = entity:getFloat("maxHealth")
    
    if health < maxHealth then
        health = health + 5.0 * deltaTime
        entity:setFloat("health", math.min(health, maxHealth))
    end
    
    -- Check death
    if health <= 0 then
        entity:deactivate()
        print(entity:getName() .. " died!")
    end
end

function onCollisionBegin(entity, event)
    local otherUID = event:getOtherEntityUID()
    -- Check if collided with damage source
    -- Take damage
    local health = entity:getFloat("health")
    entity:setFloat("health", health - 25.0)
end
```

### Example 3: Simple AI

```lua
-- scripts/enemy_ai.lua

function update(entity, deltaTime)
    local player = game:getEntity("Player")
    if not player then return end
    
    local myPos = entity:getPosition()
    local playerPos = player:getPosition()
    
    -- Calculate distance to player
    local dx = playerPos.x - myPos.x
    local dz = playerPos.z - myPos.z
    local distance = math.sqrt(dx * dx + dz * dz)
    
    local chaseRange = 15.0
    local attackRange = 2.0
    
    if distance < attackRange then
        -- Attack!
        entity:setVelocity(0, 0, 0)
        entity:setString("state", "attacking")
    elseif distance < chaseRange then
        -- Chase player
        local speed = 3.0
        local vx = (dx / distance) * speed
        local vz = (dz / distance) * speed
        entity:setVelocity(vx, 0, vz)
        entity:setString("state", "chasing")
    else
        -- Idle
        entity:setVelocity(0, 0, 0)
        entity:setString("state", "idle")
    end
end
```

---

## Best Practices

1. **Only implement functions you need** - Empty functions are not required
2. **Use script variables for state** - `entity:setFloat()`, `entity:setString()`
3. **Keep scripts simple** - Complex logic should be in C++ systems
4. **Cache expensive lookups** - Don't call `getEntity()` every frame
5. **Use deltaTime** - Make behavior frame-rate independent
6. **Check nil/null** - Always validate entity references

---

## Performance Tips

```lua
-- BAD: Creates new table every frame
function update(entity, deltaTime)
    local pos = {x = 1, y = 2, z = 3}  -- Allocates memory
    entity:setPosition(pos.x, pos.y, pos.z)
end

-- GOOD: Use existing vec3 or direct values
function update(entity, deltaTime)
    entity:setPosition(1, 2, 3)  -- No allocation
end

-- BAD: Expensive lookup every frame
function update(entity, deltaTime)
    local player = game:getEntity("Player")  -- Slow!
    -- Use player
end

-- GOOD: Cache in script variable
function onWorldLoaded(entity, event)
    local player = game:getEntity("Player")
    entity:setFloat("playerUID", player:getUID())
end

function update(entity, deltaTime)
    local playerUID = entity:getFloat("playerUID")
    -- Use cached UID
end
```