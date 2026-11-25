# Flappy Orbit artwork

`player.png` is the user's original image, moved unchanged from the game root.
`player-pixel.png` is its pixel-art adaptation, generated with built-in `image_gen`
and used in-game with per-image `pixelMode: true` for crisp sampling.
The three environment textures were generated with the built-in `image_gen`
tool, then copied here. No API/CLI fallback was used. Images are preloaded by
`src/main.ts`; UI text and panels remain native RmlUi for localization and focus.

## Final prompts

### player-pixel.png

Use case: style-transfer. Edit target: the supplied cosmic fox player sprite. Redraw this exact character as authentic hand-crafted 16-bit pixel art for a small Flappy-style game. Keep its identity, flying pose facing RIGHT, huge ears, white and lavender sci-fi armor, dark indigo outlines, cyan eyes, purple/cyan wing fins, bushy cosmic tail, star emblems and cheerful expression. Preserve approximately the original silhouette, centered position and original square canvas framing: character occupies about 85% of width and 60% height with transparent margin. Use a coarse uniform pixel grid, visually a 64 by 64 pixel sprite enlarged with nearest-neighbor to a square PNG. Intentional chunky square pixel clusters, stepped edges, limited palette around 24 colors, strong legibility at 44 pixels display size. Simplify small details tastefully while preserving the character. Sharp hard pixel edges, no antialiasing, no smooth gradients, no painted rendering, no blur, no glow outside silhouette, no text, no background, no shadow plane. Actual transparent alpha outside the character, not a checkerboard drawing. Deliver one single finished sprite, not a sheet.

### nebula.png

Use case: stylized-concept. Create a finished portrait 2D game background for a Flappy Bird style space adventure, 1024x1536. Beautiful polished hand painted cartoon science fiction matching a cute white armored cosmic fox with cyan and purple glowing wings. Deep midnight indigo space, wispy violet nebula framing the edges, delicate cyan starlight, a luminous ringed lavender planet in the UPPER RIGHT corner and tiny far moons. Quiet dark negative space through the center for legible gameplay and UI. Bottom has distant low orbital city silhouettes, no foreground ground. Rich atmospheric depth and soft bloom but crisp tasteful forms. Edge-to-edge full bleed. No characters, no pipes, no UI, no text, no letters, no logos, no borders. Background asset only.

### column.png

Use case: stylized-concept. Production game texture: one single vertical sci-fi obstacle column for a cute space Flappy Bird game. Portrait 1024x1536. The column fills the ENTIRE rectangular image from left to right and top to bottom, zero margin, no background outside the column. Front orthographic elevation, symmetrical, entirely solid and opaque. Dark midnight violet gunmetal hull with broad clean bevels and stylized white lavender alloy panel edges, luminous narrow cyan power conduits along the sides, a tasteful magenta reactor strip down the middle, layered horizontal mechanical bands at the top and bottom. The very top and very bottom are identical cyan glowing flat end caps spanning the full width, not pointed, no projections. Premium hand painted cartoon game art, strong readable large details at tiny size, softly shaded, crisp outlines. No perspective, no floor, no scene, no characters, no letters, no logos, no text. This rectangular full-bleed texture must fit a solid rectangular collider exactly.

### deck.png

Use case: stylized-concept. A horizontal full-bleed 2D game ground platform texture for a cute cosmic fox space Flappy Bird game, landscape 1536x1024. Orthographic front elevation, no perspective. The image is ENTIRELY filled with space station hull plating, with no sky or empty margin. Across the very top edge a thin brilliant cyan energy rail; under it lavender metallic beveled lip; rest of the image dark midnight indigo and violet large stylized panels, vents, soft purple energy channels and subtle cyan small lights. Restrained premium hand-painted cartoon sci-fi art with bold readable simple forms suitable for displaying as a very thin 288 by 40 game ground strip. Horizontal lines, left and right edges matching in color. No objects on top, no text, no letters, no logos, no symbols.
