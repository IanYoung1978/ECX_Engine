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
    local key = event:getKey()      -- Returns: "W", "Space", "Escape", etc.
    local pressed = event:isPressed()
    local held = event:isHeld()
    local released = event:isReleased()
end

-- Mouse Button Events (onMouseDown, onMouseUp, onMouseHeld)
function onMouseDown(entity, event)
    local button = event:getMouseButton()  -- Returns: 0=LMB, 1=RMB, 2=Middle, 3=MB4, 4=MB5, 5=Motion
    local pressed = event:mouseButtonPressed()   -- Only meaningful on onMouseDown
    local held = event:mouseButtonHeld()         -- Only meaningful on onMouseHeld
    local released = event:mouseButtonReleased() -- Only meaningful on onMouseUp
end

-- Mouse Movement Events (onMouseMove) - also delivered alongside onMouseDown/Up/Held
function onMouseMove(entity, event)
    local dx = event:getMouseMotionX()  -- Mouse movement in X since last frame
    local dy = event:getMouseMotionY()  -- Mouse movement in Y since last frame
end
```

### Collision Events

```lua
function onCollisionBegin(entity, event)
    local a = event:getCollisionEntityA()  -- Numeric entity ID of one participant
    local b = event:getCollisionEntityB()  -- Numeric entity ID of the other participant
    -- Compare against entity:getID() to work out which one is "the other guy"
end

function onCollisionEnd(entity, event)
    local a = event:getCollisionEntityA()
    local b = event:getCollisionEntityB()
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

### Appearance

```lua
local colour = entity:getColour()          -- Returns vec4 (r, g, b, a)
entity:setColour(r, g, b, a)               -- a is optional, defaults to 1.0

local blend = entity:getBlendFactor()      -- Skybox entities only; 0-1
entity:setBlendFactor(factor)              -- Clamped to 0-1
```

### Hierarchy

```lua
local hasParent = entity:hasParent()       -- true if attached to a parent
local parentID = entity:getParentID()      -- Numeric ID, 0 if none
local depth = entity:getDepth()            -- Nesting depth (0 = root)

-- Set via the game API, not on the entity itself:
game:setParent(childID, parentID)
game:clearParent(childID)
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

## vec4 API

```lua
-- Create vector (used for colours: r, g, b, a)
local c = vec4(1.0, 0.0, 0.0, 1.0)

local r, g, b, a = c.x, c.y, c.z, c.w
c.w = 0.5  -- e.g. adjust alpha
```

---

## game API Reference

### Entity Lookup

```lua
local entity = game:getEntityByName("EntityName")  -- Returns Entity (invalid if not found)
local entity = game:getEntityIDByUID(uid)           -- Returns numeric entity ID (0 if not found)
```

### System

```lua
game:shutdown()  -- Cleanly stop the engine (publishes SystemShutdown)
```

### Input

```lua
local state = game:getKeyState("W")  -- Returns KeyState int: 0=None, 1=Pressed, 2=Held, 3=Released, -1=Invalid
game:setMouseCaptured(true)          -- true = relative/FPS mouse mode (hidden, locked); false = free cursor for UI
```

### Hierarchy

```lua
game:setParent(childID, parentID)  -- Attach childID under parentID (both numeric entity IDs)
game:clearParent(childID)          -- Detach childID from its parent, resetting depth to 0
```

### Scene Management

```lua
game:loadScene("alias")      -- Begin loading a scene by its Scenes.xml alias
game:unloadScene("alias")    -- Unload a loaded scene
game:activateScene("alias")  -- Make a loaded scene the active one
```

### Graphics

```lua
game:setExposure(0.75)  -- Set HDR tonemap exposure
game:toggleDebug()       -- Toggle collider wireframe debug rendering
```

### UI (Issue #6)

UI elements are entities with `EC_UI_Element`/`EC_UI_Panel`/`EC_UI_Text` components,
authored in a UI XML file or created at runtime. Look them up by name via
`game:getEntityByName(...)`, then pass the numeric ID (`entity:getID()`) to these:

```lua
game:setUIText(entityID, "Hello")                 -- Set an EC_UI_Text element's text
game:setUITextColour(entityID, r, g, b, a)        -- Set an EC_UI_Text element's colour
game:setUIPanelColour(entityID, r, g, b, a)       -- Set an EC_UI_Panel element's colour
game:setUIVisible(entityID, true)                 -- Show/hide a UI element
game:setUIPosition(entityID, x, y)                -- Pixel-space position (top-left origin, Y-down)
game:setUISize(entityID, w, h)                     -- Pixel-space size
game:setUILayer(entityID, layer)                   -- Draw order and hit-test priority (higher on top)

local id = game:createUIElement(x, y, w, h, layer) -- Create a bare EC_UI_Element at runtime, returns its ID
```

UI elements with a `ScriptComponent` also receive their own targeted events -
`OnMouseEnter`/`OnMouseLeave`/`OnSelect`/`OnUnSelect`/`OnClick` - dispatched only to the
one element actually hovered/clicked, via the same handler-mapping table at the top of
this document (`onMouseEnter`, `onClick`, etc.).

### Debug / Diagnostics

```lua
local fps = game:getFPS()      -- EMA-smoothed frames per second
local mspf = game:getMSPF()    -- EMA-smoothed milliseconds per frame

local count = game:getRecentLogCount()      -- Number of buffered recent log lines (max 200)
local line = game:getRecentLog(index)       -- 0-based; a single recent log line as plain text

game:log("message")  -- Write an INFORMATION-level line to the engine log (visible to getRecentLog too)
```

### Ray Queries (Issue #30)

`rayQuery` is synchronous and client-initiated - it returns a count, then the results are
read back via paginated getters (avoids marshaling a vector-of-structs across the Lua
boundary). Results are sorted nearest-first unless `firstHitOnly` is set, in which case
only the single nearest hit is returned. No events are generated by the query itself.

```lua
local n = game:rayQuery(ox, oy, oz, dx, dy, dz, maxDistance, firstHitOnly)
-- ox,oy,oz    = ray origin (world space)
-- dx,dy,dz    = ray direction (normalized internally, need not be unit length)
-- maxDistance = how far along the ray to test
-- firstHitOnly = optional, default false - true returns only the nearest hit

for i = 0, n - 1 do
    local ent  = game:getRayHitEntity(i)    -- Entity hit
    local pos  = game:getRayHitPosition(i)  -- vec3 world-space hit point
    local nrm  = game:getRayHitNormal(i)    -- vec3 surface normal at the hit point
    local dist = game:getRayHitDistance(i)  -- float distance from origin
end
```

Ray-vs-shape tests every collider type (Sphere/AABB/OBB/Capsule/Cylinder/Plane) exactly,
via GJK support functions - not an approximation.

### Cone Queries (Issue #29)

`coneQuery` returns every entity whose collider shape geometrically overlaps the cone
(angle + distance), regardless of what else is in the way, unless `checkOcclusion` is
set - that opts into additionally requiring unobstructed line-of-sight to the apex (a
candidate stacked behind a closer one is excluded). Containment and occlusion are
independent, composable options, not fused together.

```lua
local n = game:coneQuery(ax, ay, az, dx, dy, dz, halfAngleDegrees, maxDistance, castsShadowOnly, checkOcclusion)
-- ax,ay,az        = cone apex (world space)
-- dx,dy,dz        = cone axis direction (normalized internally)
-- halfAngleDegrees = half-angle of the cone, in degrees
-- maxDistance      = cone height (how far the base disk sits from the apex)
-- castsShadowOnly  = optional, default true - only consider EC_DOD_GraphicsData::castsShadow entities
-- checkOcclusion   = optional, default false - also require unobstructed line-of-sight to the apex

for i = 0, n - 1 do
    local ent  = game:getConeHitEntity(i)    -- Entity found
    local pos  = game:getConeHitPosition(i)  -- vec3 world-space entity position
    local dist = game:getConeHitDistance(i)  -- float distance from apex
end
```

### Debug Visualization

Draws a wireframe for the last ray/cone query fired from script - useful to sanity-check
what a query is actually testing. Purely visual, no effect on query results; persists
until replaced by another call.

```lua
game:showDebugRay(ox, oy, oz, dx, dy, dz, maxDistance)                     -- yellow line
game:showDebugCone(ax, ay, az, dx, dy, dz, halfAngleDegrees, maxDistance)  -- magenta wireframe cone
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
    local a = event:getCollisionEntityA()
    local b = event:getCollisionEntityB()
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
    local player = game:getEntityByName("Player")
    if player:getID() == 0 then return end  -- Not found

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
4. **Cache expensive lookups** - Don't call `getEntityByName()` every frame
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
    local player = game:getEntityByName("Player")  -- Slow!
    -- Use player
end

-- GOOD: Cache in script variable
function onWorldLoaded(entity, event)
    local player = game:getEntityByName("Player")
    entity:setFloat("playerUID", player:getUID())
end

function update(entity, deltaTime)
    local playerUID = entity:getFloat("playerUID")
    -- Use cached UID
end
```