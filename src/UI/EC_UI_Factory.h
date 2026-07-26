#pragma once
#include <string>

class ECXMessenger;
class TiXmlElement;

// Dedicated XML loader for UI elements (data/scripts/XML/UI.xml) - deliberately separate from
// EC_DOD_EntityFactory (the scene-entity XML parser) so UI-specific tags (images, clickable
// regions, etc., planned but not built yet) can grow without bloating the already-large scene
// parser. Produces ordinary entities (EC_DOD_EntityInfo, EC_UI_Element, optionally EC_UI_Panel/
// EC_UI_Text/EC_DOD_ScriptData/EC_DOD_Hierarchy) via the same generic
// EC_DOD_EntityManager::createEntity()/addComponent() API the scene factory uses - nothing about
// how these entities are stored/queried/rendered/scripted is UI-specific, only how they're
// authored.
namespace EC_UI_Factory
{
    // Loads and parses `filename` once, synchronously (UI is not scene-scoped and isn't loaded
    // through the async scene-loading path - it must exist before the first frame renders).
    // Returns false (and logs) if the file can't be loaded.
    bool loadUI(const std::string& filename, ECXMessenger& messenger);
}
