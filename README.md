# Octopus
Octopus will be a note taking and PDF markup application

## Project Layers
- `base` (no namespace): Underyling functions used in all other layers
- `debug_draw` (`debug_draw_`):
    Easy to use but slow functions for debug drawing.
    Should not be used in release
- `platform` (`plat_`):
    Interact with platform-specific stuff
    (File IO, memory management, timing, etc.)
- `win` (`win_`):
    Window creation and event handling.
    Also used to initialize graphics APIs

