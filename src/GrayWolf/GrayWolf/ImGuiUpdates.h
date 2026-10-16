#pragma once

// The GUI's face of Updater: a Settings > Updates menu and the banner announcing a newer GrayWolf.

// Inside the main menu bar.
void DrawUpdatesMenu();

// Below the main menu bar, while a newer app is known and not dismissed. Returns true when the
// user chose to restart into the update (the helper is started: the caller quits).
bool DrawUpdateBanner();
