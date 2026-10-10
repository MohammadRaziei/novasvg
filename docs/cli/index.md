# CLI Reference

The `novasvg` command-line tool converts SVG files to PNG, prints information about a
document, queries elements with CSS selectors, and converts whole directories in one go.
Converting is the default action, so `novasvg input.svg` works without any subcommand.

## Synopsis

```bash
novasvg [OPTIONS] <input>              # convert (default action)
novasvg convert [OPTIONS] <input>      # same, with the explicit subcommand
novasvg info <input> [--json]          # document information
novasvg query <selector> <input> [--json]
novasvg batch <input-dir> [<output-dir>]
```

## Quick start

```bash
# Install
pip install novasvg

# Convert SVG -> PNG (writes input.png next to the input)
novasvg input.svg

# Choose the output file and size
novasvg input.svg -o output.png -w 800 -H 600
```
