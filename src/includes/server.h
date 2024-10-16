#ifndef SRC_INCLUDES_WORLD_INCL_H_
#define SRC_INCLUDES_WORLD_INCL_H_

#include "physics.h"

class ObjectFactory final
{
    private:
        static uint64_t runningId;
        static std::mutex numberIncrementMutex;

        static void processModelNode(const aiNode * node, const aiScene * scene, std::unique_ptr<PhysicsObject> & physicsObject, const std::filesystem::path & parentPath);
        static void processModelMesh(const aiMesh * mesh, const aiScene * scene, std::unique_ptr<PhysicsObject> & physicsObject, const std::filesystem::path & parentPath);
        static void processModelMeshAnimation(const aiMesh * mesh, std::unique_ptr<PhysicsObject> & physicsObject, uint32_t vertexOffset=0);

    public:
        static std::filesystem::path base;

        static const uint64_t getNextRunningId();
        static PhysicsObject * loadModel(const std::string & modelFileLocation, const std::string & id, const unsigned int importerFlags = 0, const bool useFirstChildAsRoot = false);
        static PhysicsObject * loadSphere(const std::string & id, const float radius);
        static PhysicsObject * loadBox(const std::string & id, const float & width, const float & height, const float & depth);

        static PhysicsObject * handleCreateObjectRequest(const ObjectCreateRequest * request);
        static bool handleCreateObjectResponse(CommBuilder & builder, const PhysicsObject * physicsObject);
        static bool handleCreateUpdateResponse(CommBuilder & builder, const PhysicsObject * physicsObject);
        static void addDebugResponse(CommBuilder & builder, const PhysicsObject * physicsObject);
        static PhysicsObject * handleObjectPropertiesUpdateRequest(const ObjectPropertiesUpdateRequest * request);


        static std::filesystem::path getAppPath(APP_PATHS appPath);
};


#endif

