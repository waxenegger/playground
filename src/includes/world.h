#ifndef SRC_INCLUDES_WORLD_INCL_H_
#define SRC_INCLUDES_WORLD_INCL_H_

#include "physics.h"

class ObjectFactory final
{
    public:
        static void loadModel(const std::string fileName, const std::string name, const unsigned int importerFlags, const bool useFirstChildAsRoot = false);

};


#endif

