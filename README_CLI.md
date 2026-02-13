# NovaSVG Command Line Interface

A comprehensive command-line tool for working with SVG files using the NovaSVG library.

## Features

- **Convert SVG to PNG**: Convert SVG files to PNG images with customizable dimensions
- **SVG Information**: Display detailed information about SVG files (size, bounding box, elements)
- **CSS Query**: Query and extract elements using CSS selectors
- **CSS Application**: Apply CSS stylesheets to SVG documents
- **Font Management**: Add and manage fonts for text rendering
- **Batch Processing**: Process multiple SVG files in batch mode

## Installation

### Building from Source

```bash
# Clone the repository
git clone https://github.com/MohammadRaziei/novasvg.git
cd novasvg

# Create build directory
mkdir build && cd build

# Configure with CLI enabled
cmake .. -DNOVASVG_BUILD_CLI=ON

# Build
make -j4

# The CLI executable will be available as `novasvg` in the build directory
```

### Installation System-wide

```bash
# After building
sudo make install

# The `novasvg` command will be available system-wide
```

## Usage

### Basic Commands

```bash
# Show help
novasvg --help

# Show version
novasvg --version

# Convert SVG to PNG
novasvg convert input.svg output.png

# Display SVG information
novasvg info input.svg

# Query elements with CSS selector
novasvg query "rect" input.svg

# Apply CSS stylesheet
novasvg apply-css styles.css input.svg output.png

# Add font
novasvg font add "Arial" arial.ttf

# Batch convert all SVGs in a directory
novasvg batch convert input_dir/ output_dir/
```

### Convert Command Options

```bash
# Convert with specific dimensions
novasvg convert -w 800 -H 600 input.svg output.png

# Convert with scaling
novasvg convert -s 2.0 input.svg output.png

# Convert with background color
novasvg convert -b FF0000FF input.svg output.png  # Red background

# Combine options
novasvg convert -w 400 -H 300 -b 00000000 input.svg output.png
```

### Examples

1. **Basic Conversion**:
   ```bash
   novasvg convert artwork.svg artwork.png
   ```

2. **Get SVG Information**:
   ```bash
   novasvg info document.svg
   ```

3. **Find All Rectangles**:
   ```bash
   novasvg query "rect" shapes.svg
   ```

4. **Apply Custom Styles**:
   ```bash
   novasvg apply-css custom.css input.svg styled.png
   ```

5. **Batch Process Directory**:
   ```bash
   novasvg batch convert svg_files/ png_output/
   ```

## Command Reference

### `convert`
Convert SVG file to PNG image.

**Usage**: `novasvg convert [options] <input.svg> <output.png>`

**Options**:
- `-w, --width <px>`: Output width (default: auto)
- `-H, --height <px>`: Output height (default: auto)
- `-b, --bg <color>`: Background color in hex RRGGBBAA format (default: transparent)
- `-s, --scale <factor>`: Scale factor (default: 1.0)

### `info`
Display detailed information about an SVG file.

**Usage**: `novasvg info <input.svg>`

### `query`
Query elements using CSS selectors.

**Usage**: `novasvg query "<selector>" <input.svg>`

### `apply-css`
Apply CSS stylesheet to SVG document.

**Usage**: `novasvg apply-css <styles.css> <input.svg> <output.png>`

### `font`
Manage fonts for text rendering.

**Subcommands**:
- `add <family> <filename> [bold] [italic]`: Add font face from file
- `list`: List available fonts
- `clear`: Clear font cache

### `batch`
Batch process multiple files.

**Usage**: `novasvg batch <subcommand> <input_dir> [output_dir]`

**Subcommands**:
- `convert`: Convert all SVG files in directory to PNG

## Advanced Usage

### Using with Shell Scripts

```bash
#!/bin/bash
# Convert all SVGs in current directory
for svg in *.svg; do
    novasvg convert "$svg" "${svg%.svg}.png"
done
```

### Integration with Build Systems

```cmake
# In your CMakeLists.txt
find_program(NOVASVG_CLI novasvg)
if(NOVASVG_CLI)
    add_custom_command(
        OUTPUT ${PNG_FILES}
        COMMAND ${NOVASVG_CLI} batch convert ${SVG_DIR} ${PNG_DIR}
        DEPENDS ${SVG_FILES}
    )
endif()
```

## Troubleshooting

### Common Issues

1. **Fonts not rendering correctly**:
   - Use the `font add` command to add required fonts
   - Ensure font files are accessible

2. **Large SVG files take time to process**:
   - Consider using smaller output dimensions
   - Use the `-q` (quiet) flag to suppress progress output

3. **CSS not applying as expected**:
   - Ensure CSS syntax is correct
   - Check that selectors match elements in the SVG

### Error Messages

- **"Failed to load SVG file"**: The input file doesn't exist or is not a valid SVG
- **"Failed to render SVG"**: The SVG contains unsupported features or is malformed
- **"Failed to save PNG file"**: Insufficient permissions or disk space

## Contributing

See the main project README for contribution guidelines.

## License

NovaSVG CLI is distributed under the MIT License. See the LICENSE file for details.