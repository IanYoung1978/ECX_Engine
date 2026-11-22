-- Voxel terrain shape, run once by EC_VoxelChunkSystem::init() before chunks generate (see
-- EC_LuaScriptSystem::runScriptOnce) and composed from the `volume` toolbox.
-- Rolling hills carved out of a closed box (real walls and floor). The carve is a noise-perturbed
-- halfspace subtracted from the box, so its infinite extent can't leak outside the box, and unlike a
-- raw noise field it is a true distance function that can't override the box deep inside. The box is
-- tall enough that the hills leave y in [0, 6] as an untouched wall/floor margin.
local center = vec3(16, 9, 16)
local halfExtents = vec3(24, 9, 24)
local box = volume:box(center, halfExtents)

local boxTop = 18.0 -- center.y + halfExtents.y
local hillDepth = 7.0
local hillNoiseAmplitude = 5.0

-- Solid above a noise-perturbed height threshold below the box's flat top, so the ceiling is
-- entirely replaced by the undulating surface: threshold 11 +/- 5 gives a surface in about [6, 16],
-- clear of the y:[0,6] margin. frequency 0.1 / octaves 3: the mesher samples one voxel per world
-- unit, so noise finer than about 2 units aliases into torn-looking geometry.
local carve = volume:add(
    volume:halfspace(vec3(0, boxTop - hillDepth, 0), vec3(0, -1, 0)),
    volume:scale(volume:noise(0.1, 3, 2.0, 0.5), hillNoiseAmplitude))

-- smoothSubtract rather than a hard subtract: a hard subtract against a wavy boundary leaves
-- knife-edge slivers that render as holes. The blend radius rounds them off.
volume:setRoot(volume:smoothSubtract(box, carve, 1.5))
