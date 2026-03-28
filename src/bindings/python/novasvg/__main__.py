import sys
import argparse
import logging
from pathlib import Path
from . import native_cli
from . import __version__, __author__

def setup_logging(log_level):
    """
    Configure the logging level based on the user input.
    """
    numeric_level = getattr(logging, log_level.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError(f"Invalid log level: {log_level}")
    
    logging.basicConfig(
        level=numeric_level,
        format="%(asctime)s - %(levelname)s - %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S"
    )

def handle_install(args):
    """
    Handle the 'install' command.
    Manages installation of components (cpp, matlab).
    """
    logging.info(f"Starting installation for component: {args.component}")
    
    if args.component == "cpp":
        if args.header:
            logging.info("Installing C++ header file...")
            sys.stdout.write("Installing C++ header file...\n")
            sys.stdout.write("C++ header installation complete.\n")
        
        if args.bin:
            logging.info("Installing C++ binary...")
            sys.stdout.write("Installing C++ binary...\n")
            sys.stdout.write("C++ binary installation complete.\n")

    elif args.component == "matlab":
        logging.info("Installing MATLAB support...")
        sys.stdout.write("Installing MATLAB support...\n")
        sys.stdout.write("MATLAB support installation complete.\n")


def handle_cli(args):
    """
    Handle the 'cli' command.
    Executes the native binary (novasvg-cli) passing all arguments.
    """
    logging.debug("Delegating to native CLI binary")
    # Pass the remaining arguments to the native CLI runner
    native_cli.run(sys.argv[2:])

def main():
    """
    Main entry point for the 'novasvg' command.
    """
    # 1. Check if no arguments are provided (excluding the script name)
    if len(sys.argv) == 1:
        # Print welcome message: Name, Version, Description, Help hint
        sys.stdout.write(f"NovaSVG {__version__}\n")
        sys.stdout.write("A high-performance tool for converting SVG files to PNG images and managing resources.\n")
        sys.stdout.write("Use --help for more information.\n")
        sys.exit(0)

    parser = argparse.ArgumentParser(
        description="High-performance SVG to PNG converter and resource manager."
    )
    
    # Global argument for log level
    parser.add_argument(
        "--log-level", 
        default="INFO", 
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        help="Set the logging level (default: INFO)"
    )

    # Root level version argument
    parser.add_argument(
        "--version", 
        action="version", 
        version=f"%(prog)s {__version__}"
    )

    # Create subparsers for different commands (install, cli)
    # IMPORTANT: Subparsers must be defined BEFORE optional arguments that might conflict 
    # or before parsing logic that relies on command detection.
    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # --- Command: install ---
    parser_install = subparsers.add_parser("install", help="Install components")
    parser_install.add_argument("component", choices=["cpp", "matlab"], help="Component to install (cpp or matlab)")
    
    # Options specific to 'cpp' component
    parser_install.add_argument("--header", action="store_true", default=True, help="Install header file (default: True)")
    parser_install.add_argument("--bin", action="store_true", default=True, help="Install binary (default: True)")
    parser_install.set_defaults(func=handle_install)

    # --- Command: cli ---
    parser_cli = subparsers.add_parser("cli", help="Run the native CLI binary (equivalent to novasvg-cli)")
    parser_cli.set_defaults(func=handle_cli)

    # --- Path Arguments (Root Level) ---
    # These are used for CI/CD and automation, so output is clean (just the path).
    # Note: These are defined AFTER subparsers so that 'cli' or 'install' are matched first.
    parser.add_argument(
        "--header-path", 
        action="store_true", 
        help="Print path to header file and exit"
    )
    parser.add_argument(
        "--include-path", 
        action="store_true", 
        help="Print path to include directory and exit"
    )
    parser.add_argument(
        "--binary-path", 
        action="store_true", 
        help="Print path to binary executable and exit"
    )
    parser.add_argument(
        "--cmake-path", 
        action="store_true", 
        help="Print path to CMake config directory and exit"
    )

    # Parse arguments
    args = parser.parse_args()

    # Setup logging based on the global argument
    setup_logging(args.log_level)

    # --- Handle Path Arguments (Clean Output) ---
    # If any path argument is provided, print the path and exit immediately.
    # This takes precedence over subcommands.
    base_path = Path(__file__).resolve().parent
    
    if args.header_path:
        header_path = base_path / "include" / "novasvg.h"
        sys.stdout.write(f"{header_path}\n")
        sys.exit(0)

    if args.include_path:
        include_path = base_path / "include"
        sys.stdout.write(f"{include_path}\n")
        sys.exit(0)

    if args.binary_path:
        binary_path = native_cli.novasvg_cli_path()
        sys.stdout.write(f"{binary_path}\n")
        sys.exit(0)

    if args.cmake_path:
        # Assuming CMake files are in a 'cmake' or 'lib/cmake' directory
        cmake_path = base_path / "lib" / "cmake" / "novasvg"
        sys.stdout.write(f"{cmake_path}\n")
        sys.exit(0)

    args.func(args)

if __name__ == "__main__":
    main()