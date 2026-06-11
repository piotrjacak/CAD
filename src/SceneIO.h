#pragma once
#include <string>

class Scene;

namespace sceneio {

// Serializes the scene to a JSON file
// Returns false on I/O failure.
bool save(const Scene& scene, const std::string& path);

// Clears the scene and loads it from a JSON file
// Returns false on parse/I/O failure.
bool load(Scene& scene, const std::string& path);

} // namespace sceneio
