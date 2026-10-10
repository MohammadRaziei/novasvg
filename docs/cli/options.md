# Options

## Convert (default action)

| Option | Short | Description |
|---|---|---|
| `--output <file>` | `-o` | Output PNG file (default: input name with a `.png` extension) |
| `--width <px>` | `-w` | Output width in pixels |
| `--height <px>` | `-H` | Output height in pixels |
| `--scale <factor>` | `-s` | Scale factor, e.g. `2.0` |
| `--background-color <hex>` | `-b` | Background color as `RRGGBB` or `RRGGBBAA` (default: transparent) |
| `--style <css>` | | Apply CSS styles given directly as a string |
| `--css-file <file>` | | Apply CSS styles from a file |
| `--help` | `-h` | Show help message and exit |
| `--version` | `-v` | Print version and exit |

## Subcommands

| Subcommand | Description |
|---|---|
| `convert <input>` | Convert SVG to PNG. Same as running `novasvg <input>` |
| `info <input> [--json]` | Display detailed SVG file information |
| `query <selector> <input> [--json]` | Query SVG elements using a CSS selector, e.g. `circle` or `#myId` |
| `batch <input-dir> [<output-dir>]` | Convert every SVG in a directory (default output directory: `./output`) |

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Success |
| `1` | Error (for example the input could not be loaded or the output could not be written) |
