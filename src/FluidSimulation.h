#pragma once

#include <vector>

struct DyeColour {
    float red;
    float green;
    float blue;
};

class FluidSimulation {
public:
    FluidSimulation(int width, int height, float viscosity, float dyeDiffusion);

    void addDye(int x, int y, DyeColour colour, float amount);
    void addVelocity(int x, int y, float horizontal, float vertical);
    void clear();
    void step(float timeStep);

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] DyeColour dyeAt(int x, int y) const;

private:
    int width_;
    int height_;
    float viscosity_;
    float dyeDiffusion_;

    std::vector<float> redDye_;
    std::vector<float> greenDye_;
    std::vector<float> blueDye_;
    std::vector<float> horizontalVelocity_;
    std::vector<float> verticalVelocity_;

    [[nodiscard]] int index(int x, int y) const;
    [[nodiscard]] bool inside(int x, int y) const;

    void advect(const std::vector<float>& source, std::vector<float>& destination,
                float timeStep) const;
    void diffuse(std::vector<float>& field, float amount, float timeStep) const;
    void project();
    void applySolidWalls();
};
