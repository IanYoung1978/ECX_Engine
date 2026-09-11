-- Reusable, lightweight ("arcade", not full rigid-body) gravity + ground snapping for any
-- entity with a Capsule collider. Attach via <OnUpdate alias="..."/> - state is keyed
-- per-entity-ID (not flat globals), so multiple entities can safely share this one script
-- file simultaneously, each with its own independent fall speed/grounded state.
--
-- This is the standard kinematic-character-controller technique (Quake/Source engine
-- "ground trace", Unity's CharacterController.isGrounded, Unreal's
-- CharacterMovementComponent all do a version of this - it's a solved problem, not
-- something to reinvent per-game): while GROUNDED, gravity accumulation is SUSPENDED
-- entirely - only a tiny constant "stick" speed presses the capsule toward the ground each
-- frame - and the capsule is snapped DIRECTLY to the ground surface using that frame's own
-- fresh contact query. Real accumulating gravity only resumes once ground contact is
-- actually lost (walking off a ledge, jumping, etc).
--
-- Two earlier versions of this file both got this wrong the same way, just at different
-- scales: they applied FULL accumulated gravity every frame - even while resting - and
-- then corrected the resulting penetration back out. That is fighting itself by
-- construction: any nonzero accumulated fall speed while resting means the correction has
-- real work to undo, every single frame, forever - a "large drop, large correction" cycle
-- with no fixed point, no matter how the correction formula or smoothing was tuned (one
-- version added a rolling average of contact normals across frames, which only changed
-- the cycle's period/amplitude, not its existence). Suspending gravity while grounded
-- removes the large-drop side of the cycle entirely; what's left is a millimeter-scale
-- stick bias and a direct positional snap, not a springy oscillation.
--
-- vec3 has no operator overloads in Lua (see EC_LuaScriptingSystem.cpp's registerAPI() -
-- only a constructor and x/y/z properties are bound), so position/normal math below reads
-- x/y/z fields and does plain Lua-number arithmetic rather than vector arithmetic.

local states = {}

local GRAVITY = 9.8
local TERMINAL_VELOCITY = 15.0 -- anti-tunneling cap: a discrete (non-swept) overlap test
                                -- can miss a thin surface entirely if a single frame's
                                -- fall approaches the capsule's own radius
local STICK_SPEED = 2.0 -- small constant downward bias while grounded - just enough that
                         -- the next frame's overlap query still finds the ground on a
                         -- gentle downward slope/step, without ever integrating a real
                         -- fall while resting
local GROUND_SKIN = 0.02 -- small persistent overlap kept after snapping, so the very next
                          -- frame's query still registers contact instead of landing
                          -- exactly on the boundary and flickering in and out of contact
local MIN_STANDING_NORMAL_Y = 0.3 -- how "upward-facing" a contact normal must be to count
                                    -- as ground to stand on, vs. a wall/steep slope

-- Capsule dimensions are fixed module-level constants, not read per-entity: there's no
-- Lua binding today to read a collider's own radius/height back off an entity (only to
-- author one via XML). Must match whatever <Collider><Radius>/<Height> every entity using
-- this script was actually given - currently just VoxelChunkDemo.xml's camera capsule
-- (0.5 / 2.0). A future getColliderRadius()/getColliderHeight() binding would let this
-- read real per-entity values instead.
local CAPSULE_RADIUS = 0.5
local CAPSULE_HALF_HEIGHT = 1.0

local LOG_INTERVAL = 0.5 -- throttled so this doesn't spam the log at 60+ lines/sec

-- Hysteresis on LEAVING grounded state only (entering is instant - see update() below).
-- On genuinely rough/overhung terrain (this demo's was deliberately carved with
-- smoothSubtract to have real 3D structure, not just a heightmap), the single nearest
-- triangle can be the underside of a nearby overhang lip on one frame and the flat top
-- the capsule is actually standing on the next, as it shifts by fractions of a unit under
-- the small STICK_SPEED bias - both readings are geometrically correct, but reacting to
-- either one instantly meant a single momentarily-wrong-facing contact was enough to kick
-- the capsule back to airborne, right back into the "drops, catches, drops" cycle this
-- whole redesign was meant to fix. Requiring several consecutive unusable frames before
-- actually leaving ground absorbs that single-frame noise without needing the terrain
-- itself to be simpler.
local UNGROUNDED_FRAMES_TO_RELEASE = 4

-- Live on-screen readout (see UI.xml's "gravity_debug_panel"/"gravity_debug_text") instead
-- of grepping the log file - GET /screenshot lets us watch this update frame-to-frame from
-- outside the process. Resolved lazily/once, same retry pattern as DebugOverlay.lua (entity
-- name lookup isn't populated until the scene finishes loading).
local debugTextID = nil
local debugTextReady = false

local function getState(entityId)
    local s = states[entityId]
    if not s then
        s = { fallSpeed = 0.0, grounded = false, ungroundedStreak = 0, logTimer = 0.0 }
        states[entityId] = s
    end
    return s
end

function update(entity, deltaTime)
    local entityId = entity:getID()
    local state = getState(entityId)
    local pos = entity:getPosition()
    local wasGrounded = state.grounded

    if state.grounded then
        state.fallSpeed = STICK_SPEED
    else
        state.fallSpeed = math.min(state.fallSpeed + GRAVITY * deltaTime, TERMINAL_VELOCITY)
    end

    local newY = pos.y - state.fallSpeed * deltaTime

    local hits = game:capsuleQuery(
        pos.x, newY - CAPSULE_HALF_HEIGHT, pos.z,
        pos.x, newY + CAPSULE_HALF_HEIGHT, pos.z,
        CAPSULE_RADIUS, true, entityId)

    if hits > 0 then
        local normal = game:getCapsuleHitNormal(0)
        local depth = game:getCapsuleHitDistance(0) -- penetration depth, not travel distance

        if normal.y > MIN_STANDING_NORMAL_Y then
            -- Direct snap using THIS frame's own fresh contact - not a spring correction
            -- accumulated across frames, so there's no history/averaging state to get out
            -- of sync. Correction is depth/normal.y, NOT depth*normal.y: `depth` is the
            -- distance to separate along the true contact normal, but we're only moving
            -- along Y. Moving purely in Y by dy shifts the separation along the normal by
            -- dy*normal.y (the projection of a Y-only move onto the normal) - to cancel
            -- the full `depth` this way needs dy*normal.y = depth, i.e. dy = depth/normal.y.
            local correction = (depth - GROUND_SKIN) / normal.y
            if correction > 0.0 then
                newY = newY + correction
            end
            state.grounded = true
            state.ungroundedStreak = 0
        else
            -- Touching something, but not upward-facing enough to stand on this frame (a
            -- steep slope, a wall, or - on this terrain - possibly just the underside of
            -- a nearby overhang lip) - don't snap to it, but don't necessarily leave
            -- ground either; see UNGROUNDED_FRAMES_TO_RELEASE above.
            if state.grounded then
                state.ungroundedStreak = state.ungroundedStreak + 1
                if state.ungroundedStreak >= UNGROUNDED_FRAMES_TO_RELEASE then
                    state.grounded = false
                end
            end
        end
    else
        if state.grounded then
            state.ungroundedStreak = state.ungroundedStreak + 1
            if state.ungroundedStreak >= UNGROUNDED_FRAMES_TO_RELEASE then
                state.grounded = false
            end
        end
    end

    entity:setPosition(pos.x, newY, pos.z)

    if not debugTextReady then
        local textEntity = game:getEntityByName("gravity_debug_text")
        local id = textEntity:getID()
        if id ~= 0 then
            debugTextID = id
            debugTextReady = true
        end
    end

    if debugTextReady then
        local hitInfo = "none"
        if hits > 0 then
            local n = game:getCapsuleHitNormal(0)
            hitInfo = string.format("normalY=%.3f depth=%.3f", n.y, game:getCapsuleHitDistance(0))
        end
        game:setUIText(debugTextID, string.format(
            "entity=%d y=%.3f fallSpeed=%.2f grounded=%s\nungroundedStreak=%d hits=%d %s",
            entityId, newY, state.fallSpeed, tostring(state.grounded), state.ungroundedStreak, hits, hitInfo))
    end

    -- Unthrottled, transition-only diagnostic: this script runs on the unthrottled
    -- physics/scripting thread (EC_PhysicsThreadTask::execute() has no sleep), so a
    -- throttled periodic snapshot (below) can silently skip over many real grounded/
    -- ungrounded flips between two logged samples - exactly what made the "still
    -- bouncing" behaviour hard to diagnose from the throttled log alone. This logs the
    -- instant it actually happens instead.
    if state.grounded ~= wasGrounded then
        local hitInfo = ""
        if hits > 0 then
            local n = game:getCapsuleHitNormal(0)
            hitInfo = " hitNormalY=" .. string.format("%.3f", n.y) ..
                " depth=" .. string.format("%.3f", game:getCapsuleHitDistance(0))
        end
        game:log("ArcadeGravity[" .. entityId .. "]: TRANSITION " .. tostring(wasGrounded) ..
            "->" .. tostring(state.grounded) .. " y=" .. string.format("%.3f", newY) ..
            " hits=" .. hits .. hitInfo)
    end

    state.logTimer = state.logTimer + deltaTime
    if state.logTimer >= LOG_INTERVAL then
        state.logTimer = 0.0
        game:log("ArcadeGravity[" .. entityId .. "]: y=" .. string.format("%.3f", newY) ..
            " fallSpeed=" .. string.format("%.2f", state.fallSpeed) ..
            " grounded=" .. tostring(state.grounded))
    end
end
