import os
import subprocess
import sys
from pathlib import Path

def find_vcvars():
    """Find vcvars64.bat"""
    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    possible_paths = [
        Path(program_files) / "Microsoft Visual Studio" / "2022" / "Community" / "VC" / "Auxiliary" / "Build" / "vcvars64.bat",
        Path(program_files) / "Microsoft Visual Studio" / "2022" / "Professional" / "VC" / "Auxiliary" / "Build" / "vcvars64.bat",
        Path(program_files) / "Microsoft Visual Studio" / "2022" / "Enterprise" / "VC" / "Auxiliary" / "Build" / "vcvars64.bat",
        Path(program_files) / "Microsoft Visual Studio" / "2022" / "BuildTools" / "VC" / "Auxiliary" / "Build" / "vcvars64.bat",
    ]
    
    for path in possible_paths:
        if path.exists():
            return path
    return None

def run_command(command, cwd=None, use_vcvars=False):
    """Run a command with optional Visual Studio environment"""
    print(f"Running: {' '.join(command)}")
    
    if use_vcvars:
        vcvars_path = find_vcvars()
        if not vcvars_path:
            print("Error: Could not find vcvars64.bat")
            return False
            
        # Create a batch file to run vcvars and then our command
        temp_bat = Path("temp_command.bat")
        bat_content = f'@echo off\ncall "{vcvars_path}"\n{" ".join(command)}\n'
        
        with open(temp_bat, "w") as f:
            f.write(bat_content)
        
        try:
            result = subprocess.run(str(temp_bat), cwd=cwd, check=True, text=True, capture_output=True)
            print(result.stdout)
            temp_bat.unlink()  # Clean up
            return True
        except subprocess.CalledProcessError as e:
            print(f"Error: {e}")
            print(f"Error output: {e.stderr}")
            temp_bat.unlink()  # Clean up
            return False
    else:
        try:
            result = subprocess.run(command, cwd=cwd, check=True, text=True, capture_output=True)
            print(result.stdout)
            return True
        except subprocess.CalledProcessError as e:
            print(f"Error: {e}")
            print(f"Error output: {e.stderr}")
            return False

def setup_conan():
    # Add Conancenter remote
    run_command(["conan", "remote", "add", "--force", "conancenter", "https://center.conan.io"])
    
    # Create default profile with VS environment
    run_command(["conan", "profile", "detect", "--force"], use_vcvars=True)
    
    # Get the detected profile path
    profiles_path = Path.home() / ".conan2" / "profiles"
    default_profile = profiles_path / "default"
    
    if not default_profile.exists():
        print("Error: Default profile was not created")
        return False

    # Create release profile
    release_settings = """[settings]
arch=x86_64
build_type=Release
compiler=msvc
compiler.runtime=dynamic
compiler.runtime_type=Release
compiler.version=193
os=Windows

[options]
*:shared=False

[conf]
tools.microsoft.msbuild:vs_version=193"""

    with open(profiles_path / "msvc_release", "w") as f:
        f.write(release_settings)

    # Create debug profile
    debug_settings = """[settings]
arch=x86_64
build_type=Debug
compiler=msvc
compiler.runtime=dynamic
compiler.runtime_type=Debug
compiler.version=193
os=Windows

[options]
*:shared=False

[conf]
tools.microsoft.msbuild:vs_version=193"""

    with open(profiles_path / "msvc_debug", "w") as f:
        f.write(debug_settings)

    print("\nConan profiles created:")
    print(f"Release profile: {profiles_path}/msvc_release")
    print(f"Debug profile: {profiles_path}/msvc_debug")

    # Clean Conan cache to ensure fresh builds
    run_command(["conan", "cache", "clean", "*"])

    # Create build directory
    build_dir = Path.cwd() / "build"
    if build_dir.exists():
        import shutil
        shutil.rmtree(build_dir)
    build_dir.mkdir()

    # Test install with release profile
    print("\nInstalling dependencies with Release profile...")
    if not run_command([
        "conan", "install", ".",
        "--output-folder=build",
        "--profile=msvc_release",
        "--build=missing"
    ], use_vcvars=True):
        return False

    return True

if __name__ == "__main__":
    if setup_conan():
        print("\nSetup completed successfully!")
        print("\nYou can now use these profiles with:")
        print("conan install . --output-folder=build --profile=msvc_release")
        print("conan install . --output-folder=build --profile=msvc_debug")
    else:
        print("\nSetup failed!")
        sys.exit(1)