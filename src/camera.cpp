#include "includes/engine.h"

void Camera::updateViewMatrix() {
    glm::mat4 rotM = glm::mat4(1.0f);
    rotM = glm::rotate(rotM, this->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, this->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, this->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::mat4 transM = glm::translate(glm::mat4(1.0f), -this->position);

    if (!this->isInThirdPersonMode()) {
        this->view = rotM * transM;
    } else {
        this->view = glm::lookAt(this->position, this->linkedRenderable->getBoundingSphere().center, glm::vec3(0,1,0));
    }
};

void Camera::updateFrustum() {
    const glm::mat4 matrix = this->getProjectionMatrix() * this->getViewMatrix();

    this->frustumPlanes = this->calculateFrustum(matrix);
}

const std::array<glm::vec4, 6> Camera::calculateFrustum(const glm::mat4 & matrix) {
    std::array<glm::vec4, 6> frustum;

    frustum[0].x = matrix[0].w + matrix[0].x;
    frustum[0].y = matrix[1].w + matrix[1].x;
    frustum[0].z = matrix[2].w + matrix[2].x;
    frustum[0].w = matrix[3].w + matrix[3].x;

    frustum[1].x = matrix[0].w - matrix[0].x;
    frustum[1].y = matrix[1].w - matrix[1].x;
    frustum[1].z = matrix[2].w - matrix[2].x;
    frustum[1].w = matrix[3].w - matrix[3].x;

    frustum[2].x = matrix[0].w - matrix[0].y;
    frustum[2].y = matrix[1].w - matrix[1].y;
    frustum[2].z = matrix[2].w - matrix[2].y;
    frustum[2].w = matrix[3].w - matrix[3].y;

    frustum[3].x = matrix[0].w + matrix[0].y;
    frustum[3].y = matrix[1].w + matrix[1].y;
    frustum[3].z = matrix[2].w + matrix[2].y;
    frustum[3].w = matrix[3].w + matrix[3].y;

    frustum[4].x = matrix[0].w + matrix[0].z;
    frustum[4].y = matrix[1].w + matrix[1].z;
    frustum[4].z = matrix[2].w + matrix[2].z;
    frustum[4].w = matrix[3].w + matrix[3].z;

    frustum[5].x = matrix[0].w - matrix[0].z;
    frustum[5].y = matrix[1].w - matrix[1].z;
    frustum[5].z = matrix[2].w - matrix[2].z;
    frustum[5].w = matrix[3].w - matrix[3].z;

    for (uint32_t i = 0; i < frustum.size(); i++) {
        float length = sqrtf(frustum[i].x * frustum[i].x + frustum[i].y * frustum[i].y + frustum[i].z * frustum[i].z);
        frustum[i] /= length;
    }

    return frustum;
}

const std::array<glm::vec4, 6> & Camera::getFrustumPlanes() {
    return this->frustumPlanes;
}

glm::vec3 Camera::getPosition() {
    return this->position;
}

glm::mat4 Camera::getModelMatrix() {
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    modelMatrix = glm::translate(modelMatrix, this->position);

    if (this->rotation.x != 0.0f) modelMatrix = glm::rotate(modelMatrix, this->rotation.x, glm::vec3(1, 0, 0));
    if (this->rotation.y != 0.0f) modelMatrix = glm::rotate(modelMatrix, this->rotation.y, glm::vec3(0, 1, 0));
    if (this->rotation.z != 0.0f) modelMatrix = glm::rotate(modelMatrix, this->rotation.z, glm::vec3(0, 0, 1));

    return modelMatrix;
}

bool Camera::moving()
{
    return this->keys.left || this->keys.right || this->keys.up || this->keys.down;
}

void Camera::move(KeyPress key, bool isPressed) {
    switch(key) {
        case LEFT:
            this->keys.left = isPressed;
            break;
        case RIGHT:
            this->keys.right = isPressed;
            break;
        case UP:
            this->keys.up = isPressed;
            break;
        case DOWN:
            this->keys.down = isPressed;
            break;
        default:
            break;
    }
}

void Camera::setAspectRatio(float aspect) {
    this->aspect = aspect;
    this->setPerspective();
};

void Camera::setFovY(float degrees)
{
    this->fovy = degrees;
    this->setPerspective();
};

float Camera::getFovY()
{
    return this->fovy;
};

void Camera::setPerspective()
{
    this->perspective = glm::perspective(glm::radians(this->fovy), this->aspect, 0.1f, 500.0f);
};

void Camera::setPosition(glm::vec3 position) {
    this->position = position;
}

void Camera::setRotation(glm::vec3 rotation) {
    this->rotation = rotation;
}

glm::vec3 Camera::getCameraFront() {
    glm::vec3 camFront;

    camFront.x = -cos(this->rotation.x) * sin(this->rotation.y);
    camFront.y = sin(this->rotation.x);
    camFront.z = cos(this->rotation.x) * cos(rotation.y);
    camFront = glm::normalize(camFront);

    return camFront;
}

void Camera::update(Engine * engine) {
    const auto deltaTime = engine->getRenderer()->getDeltaTime();

    if (this->deltaX != 0.0f || this->deltaY != 0.0f) {
        this->rotate(this->deltaX * CAMERA_ROTATION_PER_DELTA * (deltaTime / DELTA_TIME_60FPS), this->deltaY * CAMERA_ROTATION_PER_DELTA * (deltaTime / DELTA_TIME_60FPS));
        this->deltaX = 0.0f;
        this->deltaY = 0.0f;
    }

    glm::vec3 camFront = this->getCameraFront();
    glm::vec3 pos = this->isInThirdPersonMode() ? this->linkedRenderable->getPosition() : this->position;

    float linkedRotation = INF;

    if (moving()) {
        const float cameraMoveIncrement = CAMERA_MOVE_INCREMENT * (deltaTime / DELTA_TIME_60FPS);

        if (this->isInThirdPersonMode()) {
            linkedRotation = -this->rotation.y;
            camFront.y = 0;
        }

        if (this->keys.up) {
            pos -= camFront * cameraMoveIncrement;
        }
        if (this->keys.down) {
            pos += camFront * cameraMoveIncrement;
            linkedRotation += 2 * PI_HALF;
        }

        if (this->keys.left) {
            pos += glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f)) * cameraMoveIncrement;
            linkedRotation += this->keys.up ? PI_QUARTER : (this->keys.down ? -PI_QUARTER : PI_HALF);
        }

        if (this->keys.right) {
            pos -= glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f)) * cameraMoveIncrement;
            linkedRotation -= this->keys.up ? PI_QUARTER : (this->keys.down ? -PI_QUARTER : PI_HALF);
        }
    }

    if (this->isInThirdPersonMode()) {
        const auto oldRenderablePos = this->linkedRenderable->getPosition();
        auto oldRenderableRot = this->linkedRenderable->getRotation();
        auto linkedRenderableRot = oldRenderableRot;
        if (linkedRotation != INF) linkedRenderableRot = { 0.0f, linkedRotation, 0.0f};

        const bool hasBeenChanged = (oldRenderablePos != pos) || (linkedRenderableRot != oldRenderableRot);
        if (hasBeenChanged) {
            CommBuilder builder;
            CommCenter::addObjectPropertiesUpdateRequest(
                builder,
                this->linkedRenderable->getId(),
                { pos.x, pos.y, pos.z },
                { linkedRenderableRot.x, linkedRenderableRot.y, linkedRenderableRot.z }
            );
            CommCenter::createMessage(builder, engine->getDebugFlags());
            engine->send(builder.builder);
        }
        pos = this->position - (oldRenderablePos-pos);
    }

    this->position = pos;

    this->updateViewMatrix();
};

void Camera::linkToRenderable(Renderable* renderable)
{
    this->rotation = glm::vec3 {0.0f};
    this->linkedRenderable = renderable;

    if (renderable == nullptr) {
        this->mode = CameraMode::firstperson;
        this->position = glm::vec3{0.0f};
    } else {
        this->mode = CameraMode::lookat;
        this->rotate(0, PI_HALF/2);
    }

    this->updateViewMatrix();
}

bool Camera::isInThirdPersonMode() {
    return this->mode == CameraMode::lookat && this->linkedRenderable != nullptr;
}

void Camera::accumulateRotationDeltas(const float deltaX, const float  deltaY) {
    this->deltaX = (this->deltaX + deltaX * 0.01) / 2;
    if (glm::abs(this->deltaX) > 1.0) {
        this->deltaX = glm::sign(this->deltaX) * 1.0f;
    }

    this->deltaY = (this->deltaY + deltaY * 0.01) / 2;
    if (glm::abs(this->deltaY) > 1.0) {
        this->deltaY = glm::sign(this->deltaY) * 1.0f;
    }
}

glm::vec3 & Camera::getRotation() {
    return this->rotation;
}

void Camera::rotate(const float deltaX, const float  deltaY) {
    glm::vec3 tmpRotation(this->rotation);

    tmpRotation.x += deltaY;
    tmpRotation.y += deltaX;

    if (tmpRotation.y > 2 * glm::pi<float>()) {
        tmpRotation.y = tmpRotation.y - 2 * glm::pi<float>();
    } else if (tmpRotation.y < 0) {
        tmpRotation.y = tmpRotation.y + 2 * glm::pi<float>();
    }

    if (this->isInThirdPersonMode()) {
        tmpRotation.x = glm::clamp(tmpRotation.x, -PI_HALF / 1.5f, PI_HALF / 1.5f);

        const glm::vec3 linkedRenderableCenter = this->linkedRenderable->getBoundingSphere().center;

        this->position = {
            linkedRenderableCenter.x + DefaultThirdPersonCameraDistance * glm::cos(tmpRotation.x) * -glm::sin(tmpRotation.y),
            linkedRenderableCenter.y + DefaultThirdPersonCameraDistance * glm::sin(tmpRotation.x),
            linkedRenderableCenter.z + DefaultThirdPersonCameraDistance * glm::cos(tmpRotation.x) * glm::cos(tmpRotation.y)
        };
    } else {
        tmpRotation.x = glm::clamp(tmpRotation.x, -PI_HALF, PI_HALF);
    }

    this->rotation = tmpRotation;
}

glm::mat4 Camera::getViewMatrix() {
    return this->view;
};

glm::mat4 Camera::getProjectionMatrix() {
    return this->perspective;
};

Camera::Camera(glm::vec3 position) {
    setPosition(position);
}

Camera * Camera::INSTANCE(glm::vec3 position) {
    if (Camera::instance == nullptr) Camera::instance = new Camera(position);
    return Camera::instance;
}

Camera * Camera::INSTANCE() {
    if (Camera::instance == nullptr) Camera::instance = new Camera(glm::vec3(0.0f));
    return Camera::instance;
}

void Camera::destroy() {
    if (Camera::instance != nullptr) {
        delete Camera::instance;
        Camera::instance = nullptr;
    }
}

Camera * Camera::instance = nullptr;


