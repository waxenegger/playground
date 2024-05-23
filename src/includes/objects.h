#ifndef SRC_INCLUDES_OBJECTS_INCL_H_
#define SRC_INCLUDES_OBJECTS_INCL_H_

#include "geometry.h"

class Renderable {
    protected:
        Renderable();
    private:
        bool culled = false;
        bool inactive = false;
        bool dirty = false;
        bool registered = false;

    public:
        Renderable(const Renderable&) = delete;
        Renderable& operator=(const Renderable &) = delete;
        Renderable(Renderable &&) = delete;

        bool shouldBeRendered() const;
        void setCulled(const bool & culled);
        void setInactive(const bool & inactive);
        void setDirty(const bool & dirty);
        void flagAsRegistered();
        bool hasBeenRegistered();

        virtual ~Renderable();
};

class StaticColorVerticesRenderable : public Renderable {
    private:
        std::vector<ColorVertex> vertices;
        std::vector<uint32_t> indices;

    public:
        StaticColorVerticesRenderable(const StaticColorVerticesRenderable&) = delete;
        StaticColorVerticesRenderable& operator=(const StaticColorVerticesRenderable &) = delete;
        StaticColorVerticesRenderable(StaticColorVerticesRenderable &&) = delete;
        StaticColorVerticesRenderable();
        StaticColorVerticesRenderable(const std::vector<ColorVertex> & vertices);
        StaticColorVerticesRenderable(const std::vector<ColorVertex> & vertices, const std::vector<uint32_t> & indices);

        void setVertices(const std::vector<ColorVertex> & vertices);
        const std::vector<ColorVertex> & getVertices() const;

        void setIndices(const std::vector<uint32_t> & indices);
        const std::vector<uint32_t> & getIndices() const;

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

        void registerRenderable(Renderable * renderableObject);

        ~GlobalRenderableStore();
};

#endif



