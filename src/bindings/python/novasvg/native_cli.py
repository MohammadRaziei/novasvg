import sys
import stat
import subprocess
from pathlib import Path

def novasvg_cli_path():
    """
    Locates the 'novasvg-cli' binary relative to this file.
    Ensures executable permissions on Unix-like systems.
    Returns the Path object to the binary.
    """
    # Get the absolute path of the directory containing this file
    current_path = Path(__file__).resolve().parent
    
    # Determine the binary name based on the operating system
    binary_name = "novasvg-cli.exe" if sys.platform == "win32" else "novasvg-cli"
    
    # Construct the full path to the binary: ../bin/novasvg
    binary_path = current_path / "bin" / binary_name
    
    # Check if the binary exists
    if not binary_path.exists():
        sys.stderr.write(f"Error: CLI binary not found at expected location: {binary_path}\n")
        sys.exit(1)
    
    # Ensure the binary has executable permissions (Unix/Linux/macOS)
    # On Windows, chmod is generally not required for .exe files
    if sys.platform != "win32":
        current_mode = binary_path.stat().st_mode
        # Check if user execute bit is NOT set
        if not (current_mode & stat.S_IXUSR):
            try:
                # Add execute permission for User, Group, and Others
                binary_path.chmod(current_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
            except Exception as e:
                sys.stderr.write(f"Warning: Could not set executable permissions: {e}\n")
    
    return binary_path

def run(args: list = sys.argv[1:]):
    """
    Entry point for running 'novasvg-cli'.
    Locates the binary and executes it, passing along any arguments.
    """
    # Locate the binary using the helper function
    binary_path = novasvg_cli_path()
    
    # Execute the binary using subprocess
    try:
        # Pass the remaining arguments from sys.argv to the binary
        result = subprocess.run([str(binary_path)] + args, check=False)
        # Exit with the same code as the binary
        sys.exit(result.returncode)
    except OSError as e:
        sys.stderr.write(f"Error executing binary: {e}\n")
        sys.exit(1)
    except KeyboardInterrupt:
        # Handle Ctrl+C gracefully (Standard exit code for SIGINT is 130)
        sys.exit(130)

if __name__ == "__main__":
    run(sys.argv[1:])