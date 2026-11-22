#pragma once
#include <string>

class ECXMessenger;
class TiXmlElement;

// XML loader for UI elements (data/scripts/XML/UI.xml), separate from EC_DOD_EntityFactory so
// UI-specific tags can grow without bloating the scene parser. Produces ordinary entities via the same
// EC_DOD_EntityManager API; only their authoring is UI-specific.
namespace EC_UI_Factory
{
    // Loads and parses `filename` once, synchronously (UI is not scene-scoped and isn't loaded
    // through the async scene-loading path - it must exist before the first frame renders).
    // Returns false (and logs) if the file can't be loaded.
    bool loadUI(const std::string& filename, ECXMessenger& messenger);
}
