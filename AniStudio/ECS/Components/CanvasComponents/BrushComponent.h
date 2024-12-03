#ifndef BRUSH_COMPONENT_HPP
#define BRUSH_COMPONENT_HPP

class BrushComponent {
public:
    float size;
    float color[3];

    BrushComponent() : size(5.0f) {
        color[0] = color[1] = color[2] = 0.0f; // Default black brush
    }
};

#endif // BRUSH_COMPONENT_HPP
