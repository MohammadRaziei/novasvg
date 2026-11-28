// Built with NOVA_FOREIGN_OBJECT_SIMPLE=OFF: a document starts with no foreignObject renderer, so every
// <foreignObject> is skipped until set_foreign_object_renderer() gives it one.
#include "nova/svg/foreign_object.h"

namespace nova::svg {

std::shared_ptr<foreign_object_renderer> default_foreign_object_renderer()
{
    return nullptr;
}

} // namespace nova::svg
