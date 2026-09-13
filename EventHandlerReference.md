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
    -- Compare against entity:getID() to work out which one is "the other guy", or just:
    local otherID = event:getOtherEntityID()          -- Does that comparison for you
    local otherUID = event:entityIdToUID(otherID)     -- Numeric ID -> persistent UID (survives saves/reloads)
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

-- EC_DOD_GraphicsData - all 0/false on an entity with no GraphicsData component
local visible = entity:isVisible()             -- Pure render toggle - distinct from activate()/
entity:setVisible(false)                       -- deactivate() above, which also gates physics/collision/logic

local emissive = entity:getEmissiveIntensity()  -- Glow strength (pickups, power-state feedback)
entity:setEmissiveIntensity(2.0)

local castsShadow = entity:getCastsShadow()       -- Per-instance shadow casting toggle
entity:setCastsShadow(false)
local receivesShadow = entity:getReceivesShadow() -- Per-instance shadow receiving toggle
entity:setReceivesShadow(false)

-- EC_DOD_Skybox - degrees, rotation about world Y. Aligns the HDR panorama's baked-in sun
-- with the scene's actual directional light direction (e.g. a script-driven day/night
-- cycle) without re-exporting the HDR asset itself.
local rot = entity:getSkyboxRotation()
entity:setSkyboxRotation(45.0)
```

### Camera (EC_DOD_Camera)

```lua
-- All 0/false on an entity with no Camera component
local fov = entity:getFOV()
entity:setFOV(75.0)
local nearPlane = entity:getNearPlane()
entity:setNearPlane(0.1)
local farPlane = entity:getFarPlane()
entity:setFarPlane(500.0)

-- isCameraActive is exactly the flag the renderer checks to decide which camera(s) to draw
-- from - switching cameras (cutscene cuts, security-cam mechanics) is just flipping this on
-- the new one and off the old one; no separate "set active camera" call needed.
local active = entity:isCameraActive()
entity:setCameraActive(true)
```

### Lights (EC_DOD_Light)

```lua
-- All 0/false/(1,1,1) fallback on an entity with no Light component
local colour = entity:getLightColour()      -- vec3
entity:setLightColour(1.0, 0.5, 0.2)
local intensity = entity:getLightIntensity()
entity:setLightIntensity(2.0)

-- direction is a SEPARATE field from orientation/getForward above - see the component's
-- own comment - so spotlight sweeps need this rather than the regular orientation setter.
local dir = entity:getLightDirection()      -- vec3
entity:setLightDirection(0.0, -1.0, 0.0)

local castsShadow = entity:getLightCastsShadow()
entity:setLightCastsShadow(true)
```

### Collider (EC_DOD_Collider)

```lua
local radius = entity:getColliderRadius()   -- 0 on an entity with no Collider component
local height = entity:getColliderHeight()   -- (Sphere/Capsule/Cylinder types)

local center = entity:getColliderCenter()   -- vec3, local-space offset
entity:setColliderCenter(0.0, 1.0, 0.0)
local extents = entity:getColliderExtents() -- vec3, half-size for AABB/OBB types
entity:setColliderExtents(1.0, 2.0, 1.0)

-- Bitmask layer/mask - authored once in XML normally, but runtime-mutable here for
-- phasing/ghost-mode/"bullet ignores the shooter" patterns.
local layer = entity:getCollisionLayer()
entity:setCollisionLayer(2)
local mask = entity:getCollisionMask()
entity:setCollisionMask(0xFFFFFFFF)
```

### Hierarchy

```lua
local hasParent = entity:hasParent()       -- true if attached to a parent
local parentID = entity:getParentID()      -- Numeric ID, 0 if none
local depth = entity:getDepth()            -- Nesting depth (0 = root)

-- Walking down instead of up:
local childCount = entity:getChildCount()
local firstChildID = entity:getChildID(0)  -- 0-based index

-- Set via the game API, not on the entity itself:
game:setParent(childID, parentID)
game:clearParent(childID)
```

### Script Enable/Disable

```lua
-- EC_DOD_ScriptData::enabled - suppresses THIS entity's own event handlers (OnUpdate/
-- OnKeyDown/etc) without touching activate()/deactivate() above, e.g. a stunned/frozen
-- state that should keep rendering/colliding but stop reacting to input.
local scriptsOn = entity:isScriptEnabled()
entity:setScriptEnabled(false)
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

## vec2 API

```lua
local v = vec2(1.0, 2.0)
local x, y = v.x, v.y
v.x = 5.0
```

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
game:shutdown()      -- Cleanly stop the engine (publishes SystemShutdown)
game:pauseGame()     -- Freeze the per-tick update loop (see Startup pauseOnStart in EngineConfig.xml)
game:resumeGame()    -- Resume it
```

### Input

```lua
local state = game:getKeyState("W")  -- Returns KeyState int: 0=None, 1=Pressed, 2=Held, 3=Released, -1=Invalid
game:setMouseCaptured(true)          -- true = relative/FPS mouse mode (hidden, locked); false = free cursor for UI

-- Polling-style, distinct from the event-driven Event API above (which only reports
-- something inside a handler firing this frame) - use these from update(entity, dt) to just
-- ask "where's the mouse / is this button down right now" without waiting for an event.
local pos = game:getMousePosition()             -- vec2, screen-space pixels
local lmbDown = game:isMouseButtonPressed(0)    -- Same int convention as Event:getMouseButton() (0=LMB, 1=RMB, 2=Middle, 3=MB4, 4=MB5)
```

### Window

```lua
game:setResolution(1920, 1080)  -- Requests a window/render resolution change
game:toggleFullscreen()
game:maximizeWindow()
game:minimizeWindow()
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
local active = game:isSceneActive("alias")  -- Query current state instead of tracking it yourself
```

### Graphics

```lua
game:setExposure(0.75)      -- Set HDR tonemap exposure
game:setAmbientScale(1.0)   -- Scales the derived (from active scene lights) ambient colour
game:toggleDebug()          -- Toggle collider wireframe debug rendering
```

### Terrain

```lua
-- Re-runs the terrain generation script and re-schedules every existing voxel chunk
-- against the new shape, live. Safe to call from any script context; takes effect on the
-- next chunk-system update tick. See the `volume` API below for authoring the shape itself.
game:regenerateTerrain()
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

### Capsule Queries

Real capsule-vs-scene-geometry overlap query - includes real Mesh/terrain collision, not a
raycast stand-in. Static overlap test, not a sweep: `getCapsuleHitDistance` returns
penetration depth, and `getCapsuleHitNormal` points away from the capsule toward whatever
it's touching. Same paginated-getter pattern as ray/cone queries above.

```lua
local n = game:capsuleQuery(ax, ay, az, bx, by, bz, radius, firstHitOnly, excludeEntityId)
-- ax,ay,az / bx,by,bz = capsule segment endpoints (world space)
-- radius              = capsule radius
-- firstHitOnly        = optional, default false
-- excludeEntityId     = optional, default 0 (INVALID_ENTITY) - skips one entity (e.g. the
--                        capsule's own owner) so it doesn't find itself

for i = 0, n - 1 do
    local ent  = game:getCapsuleHitEntity(i)
    local pos  = game:getCapsuleHitPosition(i)
    local nrm  = game:getCapsuleHitNormal(i)
    local dist = game:getCapsuleHitDistance(i)  -- penetration depth, NOT travel distance
end
```

This is the technique `ArcadeGravity.lua` uses for ground snapping - see that script for a
complete worked example (ground detection, hysteresis on leaving grounded state, etc).

### Debug Visualization

Draws a wireframe for the last ray/cone query fired from script - useful to sanity-check
what a query is actually testing. Purely visual, no effect on query results; persists
until replaced by another call.

```lua
game:showDebugRay(ox, oy, oz, dx, dy, dz, maxDistance)                     -- yellow line
game:showDebugCone(ax, ay, az, dx, dy, dz, halfAngleDegrees, maxDistance)  -- magenta wireframe cone
```

### Audio (Issue #112)

Two complementary playback mechanisms sharing the same named volume categories: one-shots
for transient effects that can freely overlap (footsteps, impacts - no handle to manage),
and a single persistent slot for background music.

```lua
game:playSound("data/assets/Sounds/stepdirt_1.wav", 0.6, "sfx")  -- Fire-and-forget one-shot
game:playMusic("data/assets/Sounds/mystic_theme.mp3", 0.5, true) -- Persistent BGM slot (loop=true); replaces any currently-playing track
game:stopMusic()

-- category is any author-chosen name ("sfx", "music", ...) plus the reserved "master" name
-- for the engine's overall volume. Created on first use at full volume if it doesn't exist.
game:setCategoryVolume("sfx", 0.8)
game:setCategoryVolume("master", 1.0)
```

See `ArcadeGravity.lua` for a worked example: BGM autoplay on first scene tick, footsteps
triggered by real horizontal travel distance while grounded (not raw key input).

### Volume / Terrain Authoring (Issue #99)

A one-shot authoring toolbox for building the voxel terrain's density shape - run once by
the terrain generation script (see `EngineConfig.xml`'s `<VoxelTerrain script="...">`), not
a per-frame API. `volume` is a single global instance, called with `:` (not `.`); every
function returns/takes an opaque `VolumeHandle`. See `TerrainGeneration.lua` for a complete
real example (rolling hills carved out of a box via noise + smoothSubtract).

```lua
-- Primitive shapes - vec3 arguments, not separate x/y/z floats
local s = volume:sphere(vec3(0, 0, 0), radius)
local b = volume:box(center, halfExtents)                    -- vec3 center, vec3 half-extents
local h = volume:halfspace(pointOnPlane, normal)              -- infinite plane - see
                                                                -- TerrainGeneration.lua for
                                                                -- why this must never be the
                                                                -- base shape (unbounded in
                                                                -- the two axes along the plane)
local c = volume:cylinder(pointA, pointB, radius)              -- capsule-style: axis endpoints, not center+height
local n = volume:noise(frequency, octaves, lacunarity, persistence)  -- octaves is an int
local k = volume:constant(value)

-- Combinators - ordinary (hard-edged) and smooth (blended) boolean ops. "add" and "union"
-- are both bound (same underlying union operation, "add" is the older/shorter alias).
local u  = volume:add(a, b)
local u2 = volume:union(a, b)
local i  = volume:intersect(a, b)
local d  = volume:subtract(a, b)
local su = volume:smoothUnion(a, b, blendRadius)   -- rounds cusps instead of leaving knife-edges -
local si = volume:smoothIntersect(a, b, blendRadius) -- prefer these over the hard ops against any
local sd = volume:smoothSubtract(a, b, blendRadius)  -- noisy/wavy boundary (see TerrainGeneration.lua)

local scaled = volume:scale(a, factor)
local moved  = volume:translate(a, offsetVec3)

-- Required: register the final shape as the terrain's density field
volume:setRoot(finalShape)

-- Optional chunk configuration - see VoxelTerrainConfig for the defaults used if omitted
volume:setChunkMaterial("data/assets/shaders/basic.vert", "data/assets/shaders/PBR.frag")
volume:setChunkColour(0.4, 0.35, 0.3, 1.0)  -- r, g, b, a
volume:setGridRadius(2)  -- Chunks per axis around the origin
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