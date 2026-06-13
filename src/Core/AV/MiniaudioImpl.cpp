// The one and only translation unit that compiles miniaudio's implementation.
// All other files include MiniaudioConfig.hpp for declarations. Keeping the
// implementation isolated here guarantees a single definition of every ma_*
// symbol across the whole program (avoids multiple-definition link errors).

#ifdef _MSC_VER
#  pragma warning(push, 0)
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "Core/AV/MiniaudioConfig.hpp"

#ifdef _MSC_VER
#  pragma warning(pop)
#endif
