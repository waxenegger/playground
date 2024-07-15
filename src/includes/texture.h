#ifndef SRC_INCLUDES_TEXTURE_INCL_H_
#define SRC_INCLUDES_TEXTURE_INCL_H_

#include "shared.h"

struct TextureInformation {
    int ambientTexture = -1;
    int diffuseTexture = -1;
    int specularTexture = -1;
    int normalTexture = -1;
};

class Texture final {
    private:
        int id = 0;
        std::string type;
        std::filesystem::path path;
        bool loaded = false;
        bool valid = false;
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        SDL_Surface * textureSurface = nullptr;
        Image textureImage;

    public:
        int getId() const;
        std::string getType();
        bool isValid();
        VkFormat getImageFormat();
        void setId(const int & id);
        void setType(const std::string & type);
        void setPath(const std::filesystem::path & path);
        std::string getPath();
        void load();
        uint32_t getWidth();
        uint32_t getHeight();
        VkDeviceSize getSize();
        void * getPixels();
        void freeSurface();
        Texture(bool empty = false, VkExtent2D extent = {100, 100});
        Texture(SDL_Surface * surface);
        ~Texture();
        void cleanUpTexture(const VkDevice & device);
        bool readImageFormat();
        Image & getTextureImage();
        bool hasInitializedTextureImage();
        const VkDescriptorImageInfo getDescriptorInfo() const;
};

class Renderer;
class GlobalTextureStore final {
    private:
        static GlobalTextureStore * instance;
        GlobalTextureStore();

        std::vector<std::unique_ptr<Texture>> textures;
        std::map<const std::string, uint32_t> textureByNameLookup;

        std::mutex textureAdditionMutex;

        bool uploadTextureToGPU(Renderer * renderer, Texture * texture, const bool useAltGraphicsQueue = true);
    public:
        GlobalTextureStore& operator=(const GlobalTextureStore &) = delete;
        GlobalTextureStore(GlobalTextureStore &&) = delete;
        GlobalTextureStore & operator=(GlobalTextureStore) = delete;

        static GlobalTextureStore * INSTANCE();

        int addTexture(const std::string id, std::unique_ptr<Texture> & texture);
        int addTexture(const std::string id, const std::string fileName, const bool prefixWithAssetsImageFolder = false);
        int getOrAddTexture(const std::string id, const std::string fileName, const bool prefixWithAssetsImageFolder = false);

        bool uploadTexture(const std::string id, std::unique_ptr<Texture> & texture,  Renderer * renderer);
        bool uploadTexture(const std::string id, const std::string fileName, Renderer * renderer, const bool prefixWithAssetsImageFolder = false);

        void addDummyTexture(const std::string name = "dummy");

        uint32_t uploadTexturesToGPU(Renderer * renderer);

        Texture * getTextureByIndex(const uint32_t index);
        Texture * getTextureByName(const std::string name);
        const std::vector<std::unique_ptr<Texture>> & getTexures() const;
        const uint32_t getNumberOfTexures() const;

        void cleanUpTextures(const VkDevice & logicalDevice);

        ~GlobalTextureStore();
};

#endif


