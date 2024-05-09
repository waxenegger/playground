#include "includes/engine.h"

Shader::Shader(const VkDevice device, const std::string & filename, const VkShaderStageFlagBits shaderType) :
    filename(filename), shaderType(shaderType), device(device) {

    std::vector<char> shaderCode;
    if (!this->readFile(this->filename,shaderCode)) return;

    if (shaderCode.empty()) {
        logError("Shader File " + this->filename + " is empty!");
        return;
    }

    this->shaderModule = this->createShaderModule(shaderCode);
}

Shader::~Shader() {
    if (this->isValid()) {
        vkDestroyShaderModule(this->device, this->shaderModule, nullptr);
    }
}

std::string Shader::getFileName() const {
    return this->filename;
}

VkShaderModule Shader::createShaderModule(const std::vector<char> & code) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult ret = vkCreateShaderModule(this->device, &createInfo, nullptr, &shaderModule);

    if (ret != VK_SUCCESS) {
        logError("Failed to Create Shader Module!");
        return nullptr;
    }

    return shaderModule;
}


bool Shader::isValid() const {
        return this->shaderModule != nullptr;
}

bool Shader::readFile(const std::string & filename, std::vector<char> & buffer) {

#ifdef __ANDROID__
        unsigned long length = 0;
        const char * res = SDL_AndroidGetAssetContent(filename.c_str(), &length);

        if (length == 0) {
            logError("Failed To Read Shader File Or Shader File is empty " + filename);
            free((void *) res);
            return false;
        }

        for (int i=0;i<length;i++) {
            buffer.push_back(res[i]);
        }

        free((void *) res);
#endif

#ifndef __ANDROID__
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        logError("Failed To Open Shader File " + filename);
        return false;
    }

    size_t fileSize = (size_t) file.tellg();
    buffer.resize(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
#endif


    return true;
}

VkShaderStageFlagBits Shader::getShaderType() const {
    return this->shaderType;
}

VkShaderModule Shader::getShaderModule() const {
    return this->shaderModule;
}

