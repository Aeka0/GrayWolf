#pragma once

#include <string>
#include <vector>

// Opens an interactive Direct3D 11 window showing the given .semodel part(s): left-drag orbits,
// mouse wheel zooms, Esc or the close button quits. Draws with the shared PreviewRenderer
// (ModelPreviewGPU.h). Blocks until the window is closed.
// Returns false if the model can't be loaded or the device/window can't be created.
// Requires WraithX::InitializeAPI first (WIC, for texture decode).
bool PreviewModelInteractive(const std::vector<std::string>& SemodelPaths);
