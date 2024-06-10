#ifndef SRC_INCLUDES_MODELS_INCL_H_
#define SRC_INCLUDES_MODELS_INCL_H_

#include "texture.h"

struct NodeInformation {
    std::string name;
    glm::mat4 transformation = glm::mat4(1.0f);

    std::vector<NodeInformation> children;
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


#endif

