#include "FluidSimulation.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace {
constexpr int kSolverIterations = 30;
}

FluidSimulation::FluidSimulation(int width, int height, float viscosity, float dyeDiffusion)
    : width_(width), height_(height), viscosity_(viscosity), dyeDiffusion_(dyeDiffusion),
      redDye_(width * height, 0.0F), greenDye_(width * height, 0.0F),
      blueDye_(width * height, 0.0F), horizontalVelocity_(width * height, 0.0F),
      verticalVelocity_(width * height, 0.0F) {
    if (width < 3 || height < 3) {
        throw std::invalid_argument("The grid needs an interior surrounded by walls.");
    }
}

int FluidSimulation::index(int x, int y) const { return y * width_ + x; }

bool FluidSimulation::inside(int x, int y) const {
    return x > 0 && x < width_ - 1 && y > 0 && y < height_ - 1;
}

void FluidSimulation::addDye(int x, int y, DyeColour colour, float amount) {
    if (!inside(x, y)) return;
    const int cell = index(x, y);
    redDye_[cell] += colour.red * amount;
    greenDye_[cell] += colour.green * amount;
    blueDye_[cell] += colour.blue * amount;
}

void FluidSimulation::addVelocity(int x, int y, float horizontal, float vertical) {
    if (!inside(x, y)) return;
    const int cell = index(x, y);
    horizontalVelocity_[cell] += horizontal;
    verticalVelocity_[cell] += vertical;
}

void FluidSimulation::clear() {
    std::fill(redDye_.begin(), redDye_.end(), 0.0F);
    std::fill(greenDye_.begin(), greenDye_.end(), 0.0F);
    std::fill(blueDye_.begin(), blueDye_.end(), 0.0F);
    std::fill(horizontalVelocity_.begin(), horizontalVelocity_.end(), 0.0F);
    std::fill(verticalVelocity_.begin(), verticalVelocity_.end(), 0.0F);
}

DyeColour FluidSimulation::dyeAt(int x, int y) const {
    const int cell = index(x, y);
    return {redDye_[cell], greenDye_[cell], blueDye_[cell]};
}

void FluidSimulation::advect(const std::vector<float>& source, std::vector<float>& destination,
                             float timeStep) const {
    // Forward advection: every source cell deposits its contents at where its
    // velocity says it will arrive. Bilinear weights share it across neighbours.
    std::fill(destination.begin(), destination.end(), 0.0F);
    std::vector<float> weights(destination.size(), 0.0F);

    for (int y = 1; y < height_ - 1; ++y) {
        for (int x = 1; x < width_ - 1; ++x) {
            const int from = index(x, y);
            const float targetX = std::clamp(x + horizontalVelocity_[from] * timeStep,
                                             1.0F, static_cast<float>(width_ - 2));
            const float targetY = std::clamp(y + verticalVelocity_[from] * timeStep,
                                             1.0F, static_cast<float>(height_ - 2));
            const int left = static_cast<int>(std::floor(targetX));
            const int top = static_cast<int>(std::floor(targetY));
            const float rightWeight = targetX - left;
            const float bottomWeight = targetY - top;

            const auto deposit = [&](int toX, int toY, float weight) {
                const int to = index(toX, toY);
                destination[to] += source[from] * weight;
                weights[to] += weight;
            };
            deposit(left, top, (1.0F - rightWeight) * (1.0F - bottomWeight));
            deposit(left + 1, top, rightWeight * (1.0F - bottomWeight));
            deposit(left, top + 1, (1.0F - rightWeight) * bottomWeight);
            deposit(left + 1, top + 1, rightWeight * bottomWeight);
        }
    }

    // Multiple cells can arrive at the same destination. Averaging gives a
    // stable velocity field; density is restored to its deposited total below.
    if (&source == &horizontalVelocity_ || &source == &verticalVelocity_) {
        for (std::size_t i = 0; i < destination.size(); ++i) {
            if (weights[i] > 0.0F) destination[i] /= weights[i];
        }
    }
}

void FluidSimulation::diffuse(std::vector<float>& field, float amount, float timeStep) const {
    if (amount == 0.0F) return;

    const float coefficient = amount * timeStep;
    const float divisor = 1.0F + 4.0F * coefficient;
    const std::vector<float> original = field;
    std::vector<float> next = field;

    // Jacobi relaxation: each iteration blends a cell with its four neighbours.
    for (int iteration = 0; iteration < kSolverIterations; ++iteration) {
        for (int y = 1; y < height_ - 1; ++y) {
            for (int x = 1; x < width_ - 1; ++x) {
                const int cell = index(x, y);
                next[cell] = (original[cell] + coefficient *
                    (field[index(x - 1, y)] + field[index(x + 1, y)] +
                     field[index(x, y - 1)] + field[index(x, y + 1)])) / divisor;
            }
        }
        std::swap(field, next);
    }
}

void FluidSimulation::project() {
    std::vector<float> divergence(width_ * height_, 0.0F);
    std::vector<float> pressure(width_ * height_, 0.0F);

    for (int y = 1; y < height_ - 1; ++y) {
        for (int x = 1; x < width_ - 1; ++x) {
            divergence[index(x, y)] = -0.5F *
                (horizontalVelocity_[index(x + 1, y)] - horizontalVelocity_[index(x - 1, y)] +
                 verticalVelocity_[index(x, y + 1)] - verticalVelocity_[index(x, y - 1)]);
        }
    }

    for (int iteration = 0; iteration < kSolverIterations; ++iteration) {
        for (int y = 1; y < height_ - 1; ++y) {
            for (int x = 1; x < width_ - 1; ++x) {
                pressure[index(x, y)] = (divergence[index(x, y)] +
                    pressure[index(x - 1, y)] + pressure[index(x + 1, y)] +
                    pressure[index(x, y - 1)] + pressure[index(x, y + 1)]) * 0.25F;
            }
        }
    }

    for (int y = 1; y < height_ - 1; ++y) {
        for (int x = 1; x < width_ - 1; ++x) {
            const int cell = index(x, y);
            horizontalVelocity_[cell] -= 0.5F * (pressure[index(x + 1, y)] - pressure[index(x - 1, y)]);
            verticalVelocity_[cell] -= 0.5F * (pressure[index(x, y + 1)] - pressure[index(x, y - 1)]);
        }
    }
    applySolidWalls();
}

void FluidSimulation::applySolidWalls() {
    for (int x = 0; x < width_; ++x) {
        horizontalVelocity_[index(x, 0)] = horizontalVelocity_[index(x, height_ - 1)] = 0.0F;
        verticalVelocity_[index(x, 0)] = verticalVelocity_[index(x, height_ - 1)] = 0.0F;
    }
    for (int y = 0; y < height_; ++y) {
        horizontalVelocity_[index(0, y)] = horizontalVelocity_[index(width_ - 1, y)] = 0.0F;
        verticalVelocity_[index(0, y)] = verticalVelocity_[index(width_ - 1, y)] = 0.0F;
    }
}

void FluidSimulation::step(float timeStep) {
    std::vector<float> movedHorizontal(horizontalVelocity_.size());
    std::vector<float> movedVertical(verticalVelocity_.size());
    std::vector<float> movedDensity(redDye_.size());

    advect(horizontalVelocity_, movedHorizontal, timeStep);
    advect(verticalVelocity_, movedVertical, timeStep);
    horizontalVelocity_ = std::move(movedHorizontal);
    verticalVelocity_ = std::move(movedVertical);
    diffuse(horizontalVelocity_, viscosity_, timeStep);
    diffuse(verticalVelocity_, viscosity_, timeStep);
    applySolidWalls();
    project();

    // At 30 updates per second, 0.9848^300 is about 0.01: motion becomes
    // visually negligible after roughly ten seconds without user input.
    constexpr float kVelocityRetainedPerFrame = 0.9848F;
    for (float& value : horizontalVelocity_) {
        value *= kVelocityRetainedPerFrame;
        if (std::abs(value) < 0.0001F) value = 0.0F;
    }
    for (float& value : verticalVelocity_) {
        value *= kVelocityRetainedPerFrame;
        if (std::abs(value) < 0.0001F) value = 0.0F;
    }

    advect(redDye_, movedDensity, timeStep);
    redDye_ = std::move(movedDensity);
    movedDensity.assign(redDye_.size(), 0.0F);
    advect(greenDye_, movedDensity, timeStep);
    greenDye_ = std::move(movedDensity);
    movedDensity.assign(redDye_.size(), 0.0F);
    advect(blueDye_, movedDensity, timeStep);
    blueDye_ = std::move(movedDensity);
    diffuse(redDye_, dyeDiffusion_, timeStep);
    diffuse(greenDye_, dyeDiffusion_, timeStep);
    diffuse(blueDye_, dyeDiffusion_, timeStep);
    // Visible dye fades too: 0.99^300 is about 0.05, leaving a faint trace
    // after approximately ten seconds at 30 frames per second.
    constexpr float kDyeRetainedPerFrame = 0.99F;
    for (float& value : redDye_) value = std::max(0.0F, value * kDyeRetainedPerFrame);
    for (float& value : greenDye_) value = std::max(0.0F, value * kDyeRetainedPerFrame);
    for (float& value : blueDye_) value = std::max(0.0F, value * kDyeRetainedPerFrame);
}
