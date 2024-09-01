#ifndef SRC_INCLUDES_MODELS_INCL_H_
#define SRC_INCLUDES_MODELS_INCL_H_

#include "objects.h"

struct AnimatedModelMeshGeometry : MeshGeometry<ModelMeshIndexed> {
    std::vector<JointInformation> joints;
    std::vector<VertexJointInfo> vertexJointInfo;
    std::unordered_map<std::string, AnimationInformation> animations;
    std::unordered_map<std::string, uint32_t> jointIndexByName;
    NodeInformation rootNode;
    glm::mat4 rootInverseTransformation;
    std::string defaultAnimation = "anim0";
};

class AnimatedModelMeshRenderable : public MeshRenderable<ModelMeshIndexed, AnimatedModelMeshGeometry>, public AnimationData {
    private:
        bool needsImageSampler();
        bool needsAnimationMatrices();
    public:
        AnimatedModelMeshRenderable(const AnimatedModelMeshRenderable&) = delete;
        AnimatedModelMeshRenderable& operator=(const AnimatedModelMeshRenderable &) = delete;
        AnimatedModelMeshRenderable(AnimatedModelMeshRenderable &&) = delete;
        AnimatedModelMeshRenderable(const std::string name) : MeshRenderable<ModelMeshIndexed, AnimatedModelMeshGeometry>(name) {
            this->isAnimatedModel = true;
        };
        AnimatedModelMeshRenderable(const std::string name, const std::unique_ptr<AnimatedModelMeshGeometry> & geometry) : AnimatedModelMeshRenderable(name) {
            this->meshes = std::move(geometry->meshes);
            this->sphere = geometry->sphere;
            this->joints = std::move(geometry->joints);
            this->vertexJointInfo = std::move(geometry->vertexJointInfo);
            this->animations = std::move(geometry->animations);
            this->jointIndexByName = std::move(geometry->jointIndexByName);
            this->rootNode = std::move(geometry->rootNode);
            this->rootInverseTransformation = std::move(geometry->rootInverseTransformation);
            this->currentAnimation = std::move(geometry->defaultAnimation);
        };

        void setCurrentAnimationTime(const float time);
        void setCurrentAnimation(const std::string animation);
        const float getCurrentAnimationTime();
        const std::string getCurrentAnimation() const;
        std::vector<glm::mat4> & getAnimationMatrices();

        void dumpJointHierarchy(const uint32_t index, const uint16_t tabs);
};

using MeshRenderableVariant = std::variant<ColorMeshRenderable *, VertexMeshRenderable *, TextureMeshRenderable *, ModelMeshRenderable *,  AnimatedModelMeshRenderable *>;
using GlobalRenderableStore = GlobalObjectStore<Renderable>;

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
        static std::optional<MeshRenderableVariant> loadFromAssetsFolder(const std::string renderableName, const std::string name, const unsigned int importedFlags = 0, const bool useFirstChildAsRoot = false);
        static std::optional<MeshRenderableVariant> load(const std::string renderableName, const std::string name, const unsigned int importedFlags = 0, const bool useFirstChildAsRoot = false);
        static void addVertexJointInfo(const uint32_t jointIndex, float jointWeight, VertexJointInfo & jointInfo);
        static void processModelMeshAnimation(const aiMesh * mesh, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry, uint32_t vertexOffset=0);
        static void processAnimations(const aiScene *scene, std::unique_ptr<AnimatedModelMeshGeometry> & animatedModelMeshGeometry);

};


#endif

