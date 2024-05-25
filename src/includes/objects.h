#ifndef SRC_INCLUDES_OBJECTS_INCL_H_
#define SRC_INCLUDES_OBJECTS_INCL_H_

#include "geometry.h"

class Renderable {
    protected:
        BoundingBox bbox;

        Renderable();
        void updateMatrix();
        void updateBbox(const glm::mat4 & invMatrix = glm::mat4(0.0f));

private:
        bool inactive = false;
        bool dirty = false;
        bool registered = false;

        glm::mat4 matrix { 1.0f };
        glm::vec3 position {0.0f};
        glm::vec3 rotation { 0.0f };
        float scaling = 1.0f;

    public:
        Renderable(const Renderable&) = delete;
        Renderable& operator=(const Renderable &) = delete;
        Renderable(Renderable &&) = delete;

        bool shouldBeRendered(const std::array<glm::vec4, 6> & frustumPlanes) const;
        void setInactive(const bool & inactive);
        void setDirty(const bool & dirty);
        void flagAsRegistered();
        bool hasBeenRegistered();
        bool isInFrustum(const std::array<glm::vec4, 6> & frustumPlanes) const;

        void setPosition(const glm::vec3 & position);
        void setScaling(const float & factor);
        void setRotation(glm::vec3 & rotation);

        glm::vec3 getFront(const float leftRightAngle = 0.0f);
        void move(const float delta, const Direction & direction = { false, false, true, false });
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);

        const glm::vec3 getPosition() const;
        const float getScaling() const;
        const glm::mat4 getMatrix() const;

        virtual ~Renderable();
};

class ColorVerticesRenderable : public Renderable {
    private:
        std::vector<ColorVertex> vertices;
        std::vector<uint32_t> indices;

    public:
        ColorVerticesRenderable(const ColorVerticesRenderable&) = delete;
        ColorVerticesRenderable& operator=(const ColorVerticesRenderable &) = delete;
        ColorVerticesRenderable(ColorVerticesRenderable &&) = delete;
        ColorVerticesRenderable();
        ColorVerticesRenderable(const ColorVertexGeometry & geometry);

        void setVertices(const std::vector<ColorVertex> & vertices);
        const std::vector<ColorVertex> & getVertices() const;

        void setIndices(const std::vector<uint32_t> & indices);
        const std::vector<uint32_t> & getIndices() const;
        const BoundingBox & getBoundingBox() const;

};

class GlobalRenderableStore final {
    private:
        static GlobalRenderableStore * instance;
        GlobalRenderableStore();

        std::vector<std::unique_ptr<Renderable>> objects;

    public:
        GlobalRenderableStore& operator=(const GlobalRenderableStore &) = delete;
        GlobalRenderableStore(GlobalRenderableStore &&) = delete;
        GlobalRenderableStore & operator=(GlobalRenderableStore) = delete;

        static GlobalRenderableStore * INSTANCE();
        Renderable * getRenderableByIndex(const uint32_t & index);

        void registerRenderable(Renderable * renderableObject);

        ~GlobalRenderableStore();
};

#endif



