#ifndef SRC_INCLUDES_PHYSICS_INCL_H_
#define SRC_INCLUDES_PHYSICS_INCL_H_

#include "world.h"

class PhysicsObject {
    private:
        std::string name;

        glm::mat4 matrix { 1.0f };
        glm::vec3 position {0.0f};
        glm::vec3 rotation { 0.0f };
        float scaling = 1.0f;

        std::mutex spatialHashKeysMutex;
        std::set<std::string> spatialHashKeys;

        PhysicsObject(const std::string name);
        void updateMatrix();

    private:
        bool dirty = false;

    public:
        PhysicsObject(const PhysicsObject&) = delete;
        PhysicsObject& operator=(const PhysicsObject &) = delete;
        PhysicsObject(PhysicsObject &&) = delete;

        void setDirty(const bool & dirty);
        bool isDirty() const;

        void setPosition(const glm::vec3 position);
        void setScaling(const float factor);
        void setRotation(glm::vec3 rotation);

        glm::vec3 getUnitDirectionVector(const float leftRightAngle = 0.0f);
        void move(const float delta = 0.0f, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);

        const glm::vec3 getPosition() const;
        const glm::vec3 getRotation() const;
        const float getScaling() const;
        const glm::mat4 getMatrix() const;

        const std::set<std::string> getOrUpdateSpatialHashKeys(const bool updateHashKeys = false);

        const std::string getName() const;

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
        std::queue<PhysicsObject *> objctsToBeChecked;

        void work();
    public:
        Physics(const Physics&) = delete;
        Physics& operator=(const Physics &) = delete;
        Physics(Physics &&) = delete;
        Physics & operator=(Physics) = delete;

        Physics();

        ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> performBroadPhaseCollisionCheck();

        void checkAndResolveCollisions(const ankerl::unordered_dense::map<std::string, std::set<PhysicsObject *>> & collisions);
        void addObjectsToBeCollisionChecked(std::vector<PhysicsObject *> physicsObjects);

        void start();
        void stop();
};

using GlobalPhysicsObjectStore = GlobalObjectStore<PhysicsObject>;

#endif
