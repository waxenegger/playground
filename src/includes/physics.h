#ifndef SRC_INCLUDES_PHYSICS_INCL_H_
#define SRC_INCLUDES_PHYSICS_INCL_H_

#include "common.h"

class PhysicsObject : public AnimationData {
    private:
        std::string id;
        ObjectType type;

        KeyValueStore props;
        std::vector<Mesh> meshes;

        glm::mat4 matrix { 1.0f };
        glm::vec3 position {0.0f};
        glm::vec3 rotation { 0.0f };
        float scaling = 1.0f;

        BoundingBox originalBBox;
        BoundingBox bbox;
        BoundingSphere originalBSphere;
        BoundingSphere sphere;

        std::mutex spatialHashKeysMutex;
        std::set<std::string> spatialHashKeys;

        void updateMatrix();

        bool dirty = true;
        bool registered = false;

        void processJoints(const aiNode * node, NodeInformation & parentNode, int32_t parentIndex, bool isRoot = false);
        void processAnimations(const aiScene *scene);

    public:
        PhysicsObject(const PhysicsObject&) = delete;
        PhysicsObject& operator=(const PhysicsObject &) = delete;
        PhysicsObject(PhysicsObject &&) = delete;
        PhysicsObject(const std::string id, const ObjectType objectType);

        void setDirty(const bool & dirty);
        bool isDirty() const;
        bool doAnimationRecalculation() const;

        void setPosition(const glm::vec3 position);
        void setScaling(const float factor);
        void setRotation(glm::vec3 rotation);

        glm::vec3 getUnitDirectionVector(const float leftRightAngle = 0.0f);
        void move(const float delta = 0.0f, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);

        const glm::vec3 & getPosition() const;
        const glm::vec3 & getRotation() const;
        const float getScaling() const;
        const glm::mat4 & getMatrix() const;

        const std::set<std::string> getOrUpdateSpatialHashKeys(const bool updateHashKeys = false);
        void recalculateBoundingVolumes();
        void updateBoundingVolumes(const bool forceRecalculation = false);
        void updateBoundingSphere();
        const BoundingSphere & getBoundingSphere() const;
        const BoundingBox & getBoundingBox() const;
        bool checkBboxIntersection(const BoundingBox & otherBbox);

        const std::string getId() const;
        void flagAsRegistered();
        bool hasBeenRegistered();

        std::vector<Mesh> & getMeshes();
        void addMesh(const Mesh & mesh);
        void updateBboxWithVertex(const Vertex & vertex);
        BoundingBox getOriginalBoundingBox() const;
        void setOriginalBoundingSphere(const BoundingSphere & sphere);
        void setOriginalBoundingBox(const BoundingBox & box);
        void addVertexJointInfo(const VertexJointInfo & vertexJointInfo);
        void addJointInformation(const JointInformation & jointInfo);
        void updateVertexJointInfo(const uint32_t offset, const uint32_t jointIndex, float jointWeight);
        void updateJointIndexByName(const std::string & name, std::optional<uint32_t> value);
        const std::optional<uint32_t> getJointIndexByName(const std::string & name);

        void reserveJoints();
        void populateJoints(const aiScene * scene, const aiNode * root);

        void initProperties(const Vec3 * position, const Vec3 * rotation, const float & scale);

        template<typename T>
        T getProperty(const std::string key, T defaultValue) const
        {
            return this->props.getValue<T>(key, defaultValue);
        };
        template<typename T>
        void setProperty(const std::string key, T value)
        {
            this->props.setValue<T>(key, value);
        };

        ObjectType getObjectType() const;

        virtual ~PhysicsObject();
};

class SpatialHashMap final {
    private:
        ankerl::unordered_dense::map<std::string, std::vector<PhysicsObject *>> gridMap;

        static SpatialHashMap * instance;
        SpatialHashMap();

    public:
        SpatialHashMap& operator=(const SpatialHashMap &) = delete;
        SpatialHashMap(SpatialHashMap &&) = delete;
        SpatialHashMap & operator=(SpatialHashMap) = delete;

        static SpatialHashMap * INSTANCE();

        void addObject(PhysicsObject * physicsObject);
        void updateObject(std::set<std::string> oldIndices, std::set<std::string> newIndices, PhysicsObject * physicsObject);

        ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> performBroadPhaseCollisionCheck(const std::vector<PhysicsObject *> & physicsObject);

        ~SpatialHashMap();
};

class Physics final {
    private:
        bool quit = true;

        std::mutex additionMutex;
        std::thread worker;
        std::queue<PhysicsObject *> objctsToBeUpdated;

        void work();
    public:
        Physics(const Physics&) = delete;
        Physics& operator=(const Physics &) = delete;
        Physics(Physics &&) = delete;
        Physics & operator=(Physics) = delete;

        Physics();

        ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> performBroadPhaseCollisionCheck();

        void checkAndResolveCollisions(const ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> & collisions);
        void addObjectsToBeUpdated(std::vector<PhysicsObject *> physicsObjects);

        void start();
        void stop();
};

using GlobalPhysicsObjectStore = GlobalObjectStore<PhysicsObject>;

#endif
