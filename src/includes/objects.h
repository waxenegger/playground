#ifndef SRC_INCLUDES_OBJECTS_INCL_H_
#define SRC_INCLUDES_OBJECTS_INCL_H_

#include "geometry.h"

class Renderable {
    private:
        bool culled = false;
        bool inactive = false;
        bool dirty = false;

    public:
        Renderable(const Renderable&) = delete;
        Renderable& operator=(const Renderable &) = delete;
        Renderable(Renderable &&) = delete;

        bool shouldBeRendere() const;
        void setCulled(const bool & culled);
        void setInactive(const bool & inactive);
        void setDirty(const bool & dirty);

        virtual ~Renderable();
};

class StaticColorVerticesRenderable {
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


#endif



