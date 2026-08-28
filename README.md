# Octopus
Octopus will be a note taking and PDF markup application

## Implementation Notes

### String formatting 

I am currently playing around with the idea of having a custom string format 
schema, which works slightly differently from the stock printf. This applies 
to formatting functions as well as downstream logging functions.

### Project Layers
- `base` (no namespace):
    - Underyling functions used in all other layers
- `debug_draw` (`debug_draw_`):
    - Easy to use but slow functions for debug drawing.
    - Should not be used in release
- `platform` (`plat_`):
    - Interact with platform-specific stuff
    - (File IO, memory management, timing, etc.)
- `win` (`win_`):
    - Window creation and event handling.
    - Also used to initialize graphics APIs
- `truetype` (`tt_`):
    - Functions for working with truetype (.ttf) fonts

