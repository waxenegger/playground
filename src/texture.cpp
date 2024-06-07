#include "includes/shared.h"

int Texture::getId() const {
    return this->id;
}

std::string Texture::getType() {
    return this->type;
}

bool Texture::isValid() {
    return this->valid;
}

VkFormat Texture::getImageFormat() {
    return this->imageFormat;
}

void Texture::setId(const int & id) {
    this->id = id;
}

const VkDescriptorImageInfo Texture::getDescriptorInfo() const
{
    VkDescriptorImageInfo descriptorInfo = {};
    descriptorInfo.sampler = this->textureImage.getSampler();
    descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorInfo.imageView = this->textureImage.getImageView();

    return descriptorInfo;
}

void Texture::setType(const std::string & type) {
    this->type = type;
}

void Texture::setPath(const std::filesystem::path & path) {
    this->path = path;
}

std::string Texture::getPath() {
    return this->path.string();
}

Image & Texture::getTextureImage() {
    return this->textureImage;
}

void Texture::load() {
    if (!this->loaded) {
        this->textureSurface = IMG_Load(this->path.string().c_str());
        if (this->textureSurface != nullptr) {
            if (!this->readImageFormat()) {
                logInfo("Unsupported Texture Format: " + this->path.string());
            } else if (this->getSize() != 0) {
                this->valid = true;
            }
        } else logInfo("Failed to load texture: " + this->path.string());
        this->loaded = true;
    }
}

void Texture::cleanUpTexture(const VkDevice & device) {
    if (device == nullptr) return;

    this->textureImage.destroy(device);
}

uint32_t Texture::getWidth() {
    return this->textureSurface == nullptr ? 0 : this->textureSurface->w;
}

uint32_t Texture::getHeight() {
    return this->textureSurface == nullptr ? 0 : this->textureSurface->h;
}

VkDeviceSize Texture::getSize() {
    int channels = this->textureSurface == nullptr ? 0 : this->textureSurface->format->BytesPerPixel;
    return this->getWidth() * this->getHeight() * channels;
}

void * Texture::getPixels() {
 return this->textureSurface == nullptr ? nullptr : this->textureSurface->pixels;
}

void Texture::freeSurface() {
    if (this->textureSurface != nullptr) {
        SDL_FreeSurface(this->textureSurface);
        this->textureSurface = nullptr;
    }
}

Texture::Texture(bool empty,  VkExtent2D extent) {
    if (empty) {
        this->textureSurface = SDL_CreateRGBSurface(0,extent.width, extent.height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

        this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

        this->loaded = true;
        this->valid = this->textureSurface != nullptr;
    }
}

Texture::Texture(SDL_Surface * surface) {
    if (surface != nullptr) {
        this->textureSurface = surface;
        this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

        this->loaded = true;
        this->valid = true;
    }
}

bool Texture::hasInitializedTextureImage()
{
    return this->textureImage.isInitialized();
}

Texture::~Texture() {
    this->freeSurface();
}

bool Texture::readImageFormat() {
    if (this->textureSurface == nullptr) return false;

    const int nOfColors = this->textureSurface->format->BytesPerPixel;

    if (nOfColors == 4) {
        if (this->textureSurface->format->Rmask == 0x000000ff) this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        else this->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    } else if (nOfColors == 3) {
        Uint32 aMask = 0x000000ff;
        if (this->textureSurface->format->Rmask == 0x000000ff) this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        else {
            this->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
            aMask = 0xff000000;
        }

        // convert to 32 bit
        SDL_Surface * tmpSurface = SDL_CreateRGBSurface(
            0, this->textureSurface->w, this->textureSurface->h, 32,
            this->textureSurface->format->Rmask, this->textureSurface->format->Gmask, this->textureSurface->format->Bmask, aMask);

        // attempt twice with different pixel format
        if (tmpSurface == nullptr) {
            logError("Conversion Failed. Try something else for: " + this->path.string());

            tmpSurface = SDL_CreateRGBSurface(
                0, this->textureSurface->w, this->textureSurface->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

            if (tmpSurface == nullptr) {
                tmpSurface = SDL_CreateRGBSurface(
                    0, this->textureSurface->w, this->textureSurface->h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
            }
        }

        // either conversion has worked or not
        if (tmpSurface != nullptr) {
            SDL_SetSurfaceAlphaMod(tmpSurface, 0);
            if (SDL_BlitSurface(this->textureSurface, nullptr, tmpSurface , nullptr) == 0) {
                SDL_FreeSurface(this->textureSurface);
                this->textureSurface = tmpSurface;
            } else {
                logError("SDL_BlitSurface Failed (on conversion): " + std::string(SDL_GetError()));
                return false;
            }
        } else {
            logError("SDL_CreateRGBSurface Failed (on conversion): " + std::string(SDL_GetError()));
            return false;
        }
    } else return false;

    return true;
}

GlobalTextureStore::GlobalTextureStore() {}

GlobalTextureStore * GlobalTextureStore::INSTANCE()
{
    if (GlobalTextureStore::instance == nullptr) {
        GlobalTextureStore::instance = new GlobalTextureStore();
    }

    return GlobalTextureStore::instance;
}

int GlobalTextureStore::addTexture(const std::string id, std::unique_ptr<Texture>& texture)
{
    if (this->textureByNameLookup.contains(id) || !texture->isValid()) return -1;

    const std::lock_guard<std::mutex> lock(this->textureAdditionMutex);

    this->textures.push_back(std::move(texture));
    uint32_t index = this->textures.empty() ? 0 : this->textures.size() - 1;
    this->textureByNameLookup[id] = index;

    return index;
}

Texture * GlobalTextureStore::getTextureByIndex(const uint32_t index)
{
    if (index >= this->textures.size()) return nullptr;

    return this->textures[index].get();
}

Texture * GlobalTextureStore::getTextureByName(const std::string name)
{
    if (this->textureByNameLookup.empty()) return nullptr;

    const auto & index = this->textureByNameLookup.find(name);
    if (index == this->textureByNameLookup.end()) return nullptr;

    return this->textures[index->second].get();
}

void GlobalTextureStore::cleanUpTextures(const VkDevice& logicalDevice)
{
    logInfo("Destroying Textures...");

    this->textureByNameLookup.clear();

    for (const auto & texture : this->textures) {
        texture->cleanUpTexture(logicalDevice);
    }

    this->textures.clear();

    logInfo("Destroyed Textures");
}

GlobalTextureStore::~GlobalTextureStore() {
    if (GlobalTextureStore::instance == nullptr) return;

    delete GlobalTextureStore::instance;

    GlobalTextureStore::instance = nullptr;
}

GlobalTextureStore * GlobalTextureStore::instance = nullptr;
