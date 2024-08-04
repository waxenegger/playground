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

#include <thread>
#include <queue>
#include <set>
#include <signal.h>
#include <execution>
#include <future>

#include "unordered_dense.h"

static constexpr float PI_HALF = glm::pi<float>() / 2;
static constexpr float PI_QUARTER = PI_HALF / 2;
static constexpr float INF = std::numeric_limits<float>::infinity();
static constexpr float NEG_INF = - std::numeric_limits<float>::infinity();

struct Direction final {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};


template<typename T>
class GlobalObjectStore final {
    private:
        static GlobalObjectStore<T> * instance;
        GlobalObjectStore() {};

        std::vector<std::unique_ptr<T>> objects;
        ankerl::unordered_dense::map<std::string, uint32_t> lookupObjectsByName;
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

            const std::string name = object->getName();
            object->flagAsRegistered();
            this->objects.emplace_back(std::move(object));

            uint32_t idx = this->objects.empty() ? 1 : this->objects.size()-1;
            this->lookupObjectsByName[name] = idx;

            auto ret = this->objects[idx].get();
            //SpatialRenderableStore::INSTANCE()->addRenderable(ret);
            // TODO: physics engine sync

            return static_cast<R *>(ret);
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
        R * getObjectByName(const std::string & name) {
            const auto & hit = this->lookupObjectsByName.find(name);
            if (hit == this->lookupObjectsByName.end()) return nullptr;

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


#endif
