#include "Components/EC_DOD_Components.h"
#include "Graphics/Models/ObjModel.h"

uint32_t EC_DOD_GraphicsData::getMeshHandle() const { return model ? model->getHandle() : 0; }
uint32_t EC_DOD_GraphicsData::getVertexCount() const { return model ? model->getVertCount() : 0; }
