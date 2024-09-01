#ifndef SRC_INCLUDES_COMMON_INCL_H_
#define SRC_INCLUDES_COMMON_INCL_H_

#include "communication.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/quaternion.hpp>

#include <string>
#include <thread>
#include <queue>
#include <set>
#include <signal.h>
#include <execution>
#include <future>
#include <filesystem>
#include <any>
#include <unordered_map>

#include "unordered_dense.h"

static constexpr uint32_t MAX_JOINTS = 250;
static constexpr float PI_HALF = glm::pi<float>() / 2;
static constexpr float PI_QUARTER = PI_HALF / 2;
static constexpr float INF = std::numeric_limits<float>::infinity();
static constexpr float NEG_INF = - std::numeric_limits<float>::infinity();

static const int UNIFORM_GRID_CELL_LENGTH = 10;

enum ObjectType {
    MODEL, SPHERE, BOX
};

enum APP_PATHS {
    ROOT, TEMP, SHADERS, MODELS, IMAGES, FONTS, MAPS, MESSAGES
};

static std::filesystem::path getAppPath(std::filesystem::path base, APP_PATHS appPath) {
    switch(appPath) {
        case TEMP:
            return base / "temp";
        case SHADERS:
            return base / "shaders";
        case MODELS:
            return base / "models";
        case IMAGES:
            return base / "images";
        case FONTS:
            return base / "fonts";
        case MAPS:
            return base / "maps";
        case MESSAGES:
            return base / "messages";
        case ROOT:
        default:
            return base;
    }
}


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct Mesh {
    std::vector<Vertex> vertices;
};

struct BoundingSphere final {
    glm::vec3 center = glm::vec3(0);
    float radius = 0.0f;
};

struct BoundingBox final {
    glm::vec3 min = glm::vec3(INF);
    glm::vec3 max = glm::vec3(NEG_INF);

    BoundingSphere getBoundingSphere() const
    {
        if (this->min.x == INF || this->min.y == INF || this->min.z == INF ||
            this->max.x == NEG_INF || this->max.y == NEG_INF || this->max.z == NEG_INF) return { glm::vec3 { 0.0f }, 0.0f};

        const glm::vec3 center = {
            (this->min.x + this->max.x) / 2,
            (this->min.y + this->max.y) / 2,
            (this->min.z + this->max.z) / 2
        };

        const glm::vec3 centerToCorner = {
            this->max.x - center.x,
            this->max.y - center.y,
            this->max.z - center.z
        };

        const float r = glm::sqrt(
            centerToCorner.x * centerToCorner.x +
            centerToCorner.y * centerToCorner.y +
            centerToCorner.z * centerToCorner.z
        );

        return {center, r};
    };
};

struct Direction final {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};

struct AnimationDetailsEntry {
    double time;
    glm::vec3 scaling = glm::vec3(1.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 translation = glm::vec3(1.0f);
};


enum AnimationDetailsEntryType {
    Translation = 0, Rotation = 1, Scaling = 2
};

struct AnimationDetails {
    std::string name;
    std::vector<AnimationDetailsEntry> positions;
    std::vector<AnimationDetailsEntry> rotations;
    std::vector<AnimationDetailsEntry> scalings;

    const std::vector<AnimationDetailsEntry> getEntryDetails(const double & time, const AnimationDetailsEntryType & type) const {
        const std::vector<AnimationDetailsEntry> & entriesOfType =
            type == Translation ?
                this->positions :
                    (type == Rotation ?
                        this->rotations : this->scalings);

        std::vector<AnimationDetailsEntry> ret;

        if (entriesOfType.empty()) return ret;

        ret.resize(entriesOfType.size() == 1 ? 1 : 2);

        if (ret.size() == 1) {
            ret[0] = entriesOfType[0];
            return ret;
        }

        if (time < 0.0) {
            ret[0] = entriesOfType[0];
            ret[1] = entriesOfType[1];
            return ret;
        }

        for (uint32_t i = 0 ; i < entriesOfType.size() - 1 ; i++) {
            const AnimationDetailsEntry & e = entriesOfType[i+1];
            if (time < e.time) {
                ret[0] = entriesOfType[i];
                ret[1] = entriesOfType[i+1];
                return ret;
            }
        }

        ret[0] = entriesOfType[entriesOfType.size()-2];
        ret[1] = entriesOfType[entriesOfType.size()-1];

        return ret;
    }
};

struct AnimationInformation {
    double duration;
    double ticksPerSecond;
    std::vector<AnimationDetails> details;
};

struct JointInformation {
    std::string name;
    glm::mat4 nodeTransformation;
    glm::mat4 offsetMatrix;
    std::vector<uint32_t> children;
};

struct VertexJointInfo {
    glm::uvec4 vertexIds = glm::uvec4(0);
    glm::vec4 weights = glm::vec4(0.0f);
};

struct NodeInformation {
    std::string name;
    glm::mat4 transformation = glm::mat4(1.0f);

    std::vector<NodeInformation> children;
};

class AnimationData {
    protected:
        std::vector<JointInformation> joints;
        std::vector<VertexJointInfo> vertexJointInfo;
        std::unordered_map<std::string, AnimationInformation> animations;
        std::unordered_map<std::string, uint32_t> jointIndexByName;
        NodeInformation rootNode;
        glm::mat4 rootInverseTransformation;

        bool needsAnimationRecalculation = true;
        std::string currentAnimation = "";
        float currentAnimationTime = 0.0f;

        std::vector<glm::mat4> animationMatrices;

        void calculateJointTransformation(const std::string & animation, const float & animationTime, const NodeInformation & node, std::vector<glm::mat4> & jointTransformations, const glm::mat4 & parentTransformation) {
            glm::mat4 jointTrans = node.transformation;

            const std::string nodeName = node.name;
            const auto & animationDetails = nodeName.empty() ? std::nullopt : this->getAnimationDetails(animation, nodeName);
            if (animationDetails.has_value()) {

                glm::mat4 scalings(1);
                glm::mat4 rotations(1);
                glm::mat4 translations(1);

                // add scaling, translation etc
                const std::vector<AnimationDetailsEntry> animationScalings = animationDetails->getEntryDetails(animationTime, Scaling);
                if (!animationScalings.empty()) {
                    if (animationScalings.size() == 1) {
                        scalings = glm::scale(scalings, animationScalings[0].scaling);
                    } else {
                        double deltaTime = animationScalings[1].time - animationScalings[0].time;
                        float deltaFactor = static_cast<float>((animationTime - animationScalings[0].time) / deltaTime);
                        scalings = glm::scale(scalings, animationScalings[0].scaling + (animationScalings[1].scaling - animationScalings[0].scaling) * deltaFactor);
                    }
                }

                const std::vector<AnimationDetailsEntry> animationRotations = animationDetails->getEntryDetails(animationTime, Rotation);
                if (!animationRotations.empty()) {
                    if (animationRotations.size() == 1) {
                        rotations = glm::toMat4(animationRotations[0].rotation);
                    } else {
                        double deltaTime = animationRotations[1].time - animationRotations[0].time;
                        float deltaFactor = static_cast<float>((animationTime - animationRotations[0].time) / deltaTime);
                        rotations = glm::toMat4(glm::slerp(animationRotations[0].rotation, animationRotations[1].rotation, deltaFactor));
                    }
                }

                const std::vector<AnimationDetailsEntry> animationTranslations = animationDetails->getEntryDetails(animationTime, Translation);
                if (!animationTranslations.empty()) {
                    if (animationRotations.size() == 1) {
                        translations = glm::translate(translations, animationTranslations[0].translation);
                    } else {
                        double deltaTime = animationTranslations[1].time - animationTranslations[0].time;
                        float deltaFactor = static_cast<float>((animationTime - animationTranslations[0].time) / deltaTime);
                        translations = glm::translate(translations, animationTranslations[0].translation + (animationTranslations[1].translation - animationTranslations[0].translation) * deltaFactor);
                    }
                }

                jointTrans = translations * rotations * scalings;
            }

            glm::mat4 trans = parentTransformation * jointTrans;

            if (this->jointIndexByName.contains(nodeName)) {
                const uint32_t jointIndex = this->jointIndexByName[nodeName];
                const JointInformation animatedJoint = this->joints[jointIndex];
                jointTransformations[jointIndex] = this->rootInverseTransformation * trans * animatedJoint.offsetMatrix;
            }

            for (const auto & child : node.children) {
                this->calculateJointTransformation(animation, animationTime, child, jointTransformations, trans);
            }
        };

        std::optional<AnimationDetails> getAnimationDetails(const std::string & animation, const std::string & jointName) {
            if (!this->animations.contains(animation)) return std::nullopt ;

            const AnimationInformation & animations = this->animations[animation];
            for(auto & animationDetail : animations.details) {
                if (animationDetail.name == jointName) {
                    return animationDetail;
                }
            }

            return std::nullopt;
        };

    public:
        std::string getCurrentAnimation() const
        {
            return this->currentAnimation;
        };

        float getCurrentAnimationTime() const
        {
            return this->currentAnimationTime;
        };

        void setCurrentAnimationTime(const float time) {
            if (!this->animations.contains(this->currentAnimation) || time == this->currentAnimationTime) return;

            const float animationDuration = this->animations[this->currentAnimation].duration;

            float changedTime = time;
            if (changedTime < 0.0f || changedTime > animationDuration) changedTime = 0.0f;

            this->currentAnimationTime = changedTime;

            this->needsAnimationRecalculation = true;
        }

        void setCurrentAnimation(const std::string animation) {
            if (!this->animations.contains(this->currentAnimation) || animation == this->currentAnimation) return;

            this->currentAnimation = animation;
            this->currentAnimationTime = 0.0f;

            this->needsAnimationRecalculation = true;
        }

        bool calculateAnimationMatrices() {
            if (!this->needsAnimationRecalculation || !this->animations.contains(this->currentAnimation) ) {
                this->needsAnimationRecalculation = false;
                return false;
            }

            this->animationMatrices = std::vector<glm::mat4>(this->vertexJointInfo.size(), glm::mat4(1.0f));

            std::vector<glm::mat4> jointTransforms = std::vector<glm::mat4>(this->joints.size(), glm::mat4(1.0f));

            this->calculateJointTransformation(this->currentAnimation, this->currentAnimationTime, this->rootNode, jointTransforms, glm::mat4(1));

            for (uint32_t i=0;i<this->vertexJointInfo.size();i++) {
                const VertexJointInfo & jointInfo = this->vertexJointInfo[i];
                glm::mat4 jointTransform = glm::mat4(1.0f);

                if (jointInfo.weights.x > 0.0) {
                    jointTransform += jointTransforms[jointInfo.vertexIds.x] * jointInfo.weights.x;
                }

                if (jointInfo.weights.y > 0.0) {
                    jointTransform += jointTransforms[jointInfo.vertexIds.y] * jointInfo.weights.y;
                }

                if (jointInfo.weights.z > 0.0) {
                    jointTransform += jointTransforms[jointInfo.vertexIds.z] * jointInfo.weights.z;
                }

                if (jointInfo.weights.w > 0.0) {
                    jointTransform += jointTransforms[jointInfo.vertexIds.w] * jointInfo.weights.w;
                }

                this->animationMatrices[i] = jointTransform;
            }

            this->needsAnimationRecalculation = false;

            return true;
        }
};

template<typename T>
class GlobalObjectStore {
    protected:
        static GlobalObjectStore<T> * instance;
        GlobalObjectStore() {};

        std::vector<std::unique_ptr<T>> objects;
        ankerl::unordered_dense::map<std::string, uint32_t> lookupObjectsById;
        std::mutex registrationMutex;

    public:
        GlobalObjectStore<T>& operator=(const GlobalObjectStore<T> &) = delete;
        GlobalObjectStore(GlobalObjectStore<T> &&) = delete;
        GlobalObjectStore<T> & operator=(GlobalObjectStore<T>) = delete;

        static GlobalObjectStore<T> * INSTANCE() {
            if (GlobalObjectStore<T>::instance == nullptr) {
                GlobalObjectStore<T>::instance = new GlobalObjectStore<T>();
            }
            return GlobalObjectStore<T>::instance;
        };

        template<typename R>
        R * registerObject(std::unique_ptr<R> & object) {
            const std::lock_guard<std::mutex> lock(this->registrationMutex);

            const std::string id = object->getId();
            object->flagAsRegistered();
            this->objects.emplace_back(std::move(object));

            uint32_t idx = this->objects.empty() ? 1 : this->objects.size()-1;
            this->lookupObjectsById[id] = idx;

            return static_cast<R *>(this->objects[idx].get());
        }

        template<typename R>
        R * getObjectByIndex(const uint32_t & index) {
            if (index >= this->objects.size()) return nullptr;

            try {
                return dynamic_cast<R *>(this->objects[index].get());
            } catch(std::bad_cast wrongTypeException) {
                return nullptr;
            }

            return nullptr;
        };

        template<typename R>
        R * getObjectById(const std::string & id) {
            const auto & hit = this->lookupObjectsById.find(id);
            if (hit == this->lookupObjectsById.end()) return nullptr;

            return this->getObjectByIndex<R>(hit->second);
        };

        void performFrustumCulling(const std::array<glm::vec4, 6> & frustumPlanes) {
            const std::lock_guard<std::mutex> lock(this->registrationMutex);

            std::for_each(
                std::execution::par,
                this->objects.begin(),
                this->objects.end(),
                [&frustumPlanes](auto & object) {
                    object->performFrustumCulling(frustumPlanes);
                }
            );
        };

        uint32_t getNumberOfObjects() {
            return this->objects.size();
        };

        std::vector<std::unique_ptr<T>> getObjects() {
            return this->objects;
        };

        ~GlobalObjectStore() {
            if (GlobalObjectStore<T>::instance == nullptr) return;

            this->objects.clear();
            delete GlobalObjectStore<T>::instance;
            GlobalObjectStore<T>::instance = nullptr;
        };
};

template<typename T>
GlobalObjectStore<T> * GlobalObjectStore<T>::instance = nullptr;

class KeyValueStore final {
    private:
        std::unordered_map<std::string, std::any> map;
    public:
        KeyValueStore& operator=(const KeyValueStore &) = delete;
        KeyValueStore(KeyValueStore &&) = delete;
        KeyValueStore & operator=(KeyValueStore) = delete;
        KeyValueStore() {};

        template <typename T>
        T getValue(const std::string &key, T defaultValue) const
        {
            auto it = this->map.find(key);
            if (it == this->map.end() || !it->second.has_value()) return defaultValue;

            T ret;
            try {
                ret = std::any_cast<T>(it->second);
            } catch(std::bad_any_cast ex) {
                logError("Failed to cast map value to given type!");
                return defaultValue;
            }

            return ret;
        };

        template <typename T>
        void setValue(const std::string key, T value)
        {
            this->map[key] = value;
        };

        uint32_t getSize() const {
            return this->map.size();
        };
};


#endif
