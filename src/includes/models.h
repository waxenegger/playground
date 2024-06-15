#ifndef SRC_INCLUDES_MODELS_INCL_H_
#define SRC_INCLUDES_MODELS_INCL_H_

#include "objects.h"

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

        for (uint i = 0 ; i < entriesOfType.size() - 1 ; i++) {
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

struct AnimatedModelMeshGeometry : MeshGeometry<AnimatedModelMeshIndexed> {
    std::vector<JointInformation> joints;
    std::vector<VertexJointInfo> vertexJointInfo;
    std::map<std::string, AnimationInformation> animations;
    std::map<std::string, uint32_t> jointIndexByName;
    NodeInformation rootNode;
    glm::mat4 rootInverseTransformation;
    std::string defaultAnimation = "anim0";
};

class AnimatedModelMeshRenderable : public MeshRenderable<AnimatedModelMeshIndexed, AnimatedModelMeshGeometry> {
    private:
        std::vector<JointInformation> joints;
        std::vector<VertexJointInfo> vertexJointInfo;
        std::map<std::string, AnimationInformation> animations;
        std::map<std::string, uint32_t> jointIndexByName;
        NodeInformation rootNode;
        glm::mat4 rootInverseTransformation;

        bool needsAnimationRecalculation = true;
        std::string currentAnimation = "anim0";
        float currentAnimationTime = 0.0f;

        std::vector<glm::mat4> animationMatrices;

        void calculateJointTransformation(const std::string & animation, const float & animationTime, const NodeInformation & node, std::vector<glm::mat4> & jointTransformations, const glm::mat4 & parentTransformation);
        std::optional<AnimationDetails>getAnimationDetails(const std::string & animation, const std::string & jointName);

        bool needsImageSampler();
        bool needsAnimationMatrices();
    public:
        AnimatedModelMeshRenderable(const AnimatedModelMeshRenderable&) = delete;
        AnimatedModelMeshRenderable& operator=(const AnimatedModelMeshRenderable &) = delete;
        AnimatedModelMeshRenderable(AnimatedModelMeshRenderable &&) = delete;
        AnimatedModelMeshRenderable(const std::unique_ptr<AnimatedModelMeshGeometry> & geometry) {
            this->meshes = std::move(geometry->meshes);
            this->bbox = geometry->bbox;
            this->joints = std::move(geometry->joints);
            this->vertexJointInfo = std::move(geometry->vertexJointInfo);
            this->animations = std::move(geometry->animations);
            this->jointIndexByName = std::move(geometry->jointIndexByName);
            this->rootNode = std::move(geometry->rootNode);
            this->rootInverseTransformation = std::move(geometry->rootInverseTransformation);
            this->currentAnimation = std::move(geometry->defaultAnimation);
        };

        void changeCurrentAnimationTime(const float time);
        void setCurrentAnimation(const std::string animation);
        const float getCurrentAnimationTime();
        const std::string getCurrentAnimation() const;
        std::vector<glm::mat4> & getAnimationMatrices();

        void dumpJointHierarchy(const uint32_t index, const uint16_t tabs);
        bool calculateAnimationMatrices();
};

using MeshRenderableVariant = std::variant<ColorMeshRenderable *, VertexMeshRenderable *, TextureMeshRenderable *, ModelMeshRenderable *,  AnimatedModelMeshRenderable *>;

class Model final {
    private:
        Model();
        Model& operator=(const Model &) = delete;
        Model(Model &&) = delete;
        Model & operator=(Model) = delete;

        static void correctTexturePath(char * path);
        static std::string saveEmbeddedModelTexture(const aiTexture * texture);

        static void processModelNode(const aiNode * node, const aiScene * scene, std::unique_ptr<ModelMeshGeometry> & modelMeshGeom, const std::filesystem::path & parentPath);
        static void processModelNode(const aiNode * node, const aiScene * scene, std::unique_ptr<AnimatedModelMeshGeometry> & modelMeshGeom, const std::filesystem::path & parentPath);

        template <typename G, typename M>
        static void processModelMesh(const aiMesh * mesh, const aiScene * scene, std::unique_ptr<G> & modelMeshGeom, const std::filesystem::path & parentPath);

        static void processMeshTexture(const aiMaterial * mat, const aiScene * scene, TextureInformation & meshTextureInfo, const std::filesystem::path & parentPath);
        static void processJoints(const aiNode * node, NodeInformation & parentNode, int32_t parentIndex, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry, bool isRoot = false);

    public:
        static std::optional<MeshRenderableVariant> loadFromAssetsFolder(const std::string name, const unsigned int importedFlags = 0, const bool useFirstChildAsRoot = false);
        static std::optional<MeshRenderableVariant> load(const std::string name, const unsigned int importedFlags = 0, const bool useFirstChildAsRoot = false);
        static void addVertexJointInfo(const uint32_t jointIndex, float jointWeight, VertexJointInfo & jointInfo);
        static void processModelMeshAnimation(const aiMesh * mesh, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry, uint32_t vertexOffset=0);
        static void processAnimations(const aiScene *scene, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry);

};


#endif

