-- Voxel chunk world shape - author-authored replacement for the old hardcoded
-- EC_TerrainWorldDensity C++ formula. Run once by EC_VoxelChunkSystem::init() before any
-- chunk is generated (not a per-frame/per-event handler script - see
-- EC_LuaScriptSystem::runScriptOnce). Composed entirely from the `volume` toolbox
-- (Stage 1/2 of the terrain generation plan: "create a volume, subtract from it") - no
-- engine C++ edits needed to change this shape.
--
-- Rolling hills, carved subtractively out of a genuinely closed box (real walls, real
-- floor - see this file's own history for why `box`, not `halfspace`, has to be the base
-- shape: a halfspace is an infinite plane, and nothing bounds it in X/Z on its own).
-- Using a noise-perturbed halfspace as the thing being SUBTRACTED, rather than as the
-- base shape itself, is safe: subtraction only ever removes material that's already
-- inside the box, so the carve tool's own unboundedness in X/Z can't leak out and
-- un-bound the result - and unlike a raw noise field combined directly via CSG (the
-- domination bug found earlier), a halfspace is a true distance function whose magnitude
-- keeps growing away from its own plane, so it can't spuriously override the box's shape
-- deep inside where we don't want any carving to happen at all.
-- Taller than the original box (was half-extent 6 / height 12) to give pronounced hills
-- enough vertical room without the carve's range reaching all the way down to the walls -
-- an earlier attempt at "more pronounced" widened the noise amplitude enough that valleys
-- dipped low enough to erode the mid-height walls too, not just the top (a real geometric
-- consequence of tall relief inside a shallow box, not a bug, but not what was wanted
-- here). height 18 with the range below leaves y:[0,6] as a clean, untouched wall/floor
-- margin everywhere.
local center = vec3(16, 9, 16)
local halfExtents = vec3(24, 9, 24)
local box = volume:box(center, halfExtents)

local boxTop = 18.0 -- center.y + halfExtents.y
local hillDepth = 7.0
local hillNoiseAmplitude = 5.0

-- Solid (and therefore carved away from the box) above a Perlin-noise-perturbed height
-- threshold a few units below the box's own flat top - the box's true ceiling never
-- survives untouched, it's entirely replaced by this undulating surface. Threshold 11.0
-- +/- amplitude 5.0 => surface roughly in [6, 16], a 10-unit range (was a 4-unit range
-- before) for real, dramatic relief, while never approaching the clean y:[0,6] wall/floor
-- margin.
--
-- frequency 0.1 / octaves 3 (was 0.16 / 4): the marching-cubes grid samples one voxel per
-- world unit, so any noise detail finer than roughly 2 world units aliases - visible as
-- torn/cracked-looking geometry and broken shading, not genuine terrain detail. The
-- previous settings' finest octave (0.16 * 2^3 = 1.28, a ~0.8-unit period) was well past
-- that limit. Dropping to frequency 0.1 / octaves 3 keeps the finest octave's period
-- around 2.5 units - comfortably resolvable - while the large-scale relief (what actually
-- reads as "pronounced hills") barely changes, since that comes from the low-frequency
-- octaves and the amplitude, not the fine chatter on top.
local carve = volume:add(
    volume:halfspace(vec3(0, boxTop - hillDepth, 0), vec3(0, -1, 0)),
    volume:scale(volume:noise(0.1, 3, 2.0, 0.5), hillNoiseAmplitude))

-- smoothSubtract, not a hard subtract: a hard CSG subtract against a wavy, noisy boundary
-- creates arbitrarily thin, knife-edge slivers wherever the carve surface grazes the box
-- at a shallow angle - degenerate-thin geometry that showed up as actual triangular holes
-- in the rendered mesh (confirmed via GET /screenshot?target=depth: those spots read as
-- pure background/max depth, i.e. real gaps, not a shading bug). A modest blend radius
-- rounds off those cusps instead of leaving a knife edge.
volume:setRoot(volume:smoothSubtract(box, carve, 1.5))
