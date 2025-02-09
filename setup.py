import os
import subprocess
import sys
import shutil
from pathlib import Path

def find_visual_studio():
    """Find Visual Studio installation"""
    possible_programs = [
        os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
        os.environ.get("ProgramFiles", r"C:\Program Files"),
    ]
    
    for program_files in possible_programs:
        base_path = Path(program_files) / "Microsoft Visual Studio"
        if base_path.exists():
            for edition in ["Enterprise", "Professional", "Community", "BuildTools"]:
                vs_path = base_path / "2022" / edition
                if vs_path.exists():
                    return vs_path
    return None

def create_profile(profile_path):
    """Create a Visual Studio Release profile"""
    content = """[settings]
arch=x86_64
build_type=Release
compiler=msvc
compiler.runtime=dynamic
compiler.runtime_type=Release
compiler.version=193
os=Windows

[buildenv]
CC=cl.exe
CXX=cl.exe"""
    
    profile_path.parent.mkdir(parents=True, exist_ok=True)
    with open(profile_path, 'w') as f:
        f.write(content)
    print(f"Created profile at: {profile_path}")

def run_command(command, cwd=None):
    """Run a command and print its output"""
    try:
        print(f"Running command: {' '.join(command)}")
        result = subprocess.run(
            command,
            cwd=cwd,
            check=True,
            text=True,
            capture_output=True,
            env=os.environ.copy()
        )
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {' '.join(command)}")
        print(f"Error output: {e.stderr}")
        return False

def setup_project():
    # Get the project root directory
    project_root = Path.cwd()
    build_dir = project_root / "build"
    
    # Create build directory if it doesn't exist
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir()
    
    # Create temporary profiles in the project directory
    temp_profiles_dir = project_root / "profiles"
    temp_profiles_dir.mkdir(exist_ok=True)
    
    # Create Release profile
    profile_path = temp_profiles_dir / "vs2022_release"
    create_profile(profile_path)
    
    # Run conan install for Release
    if not run_command([
        "conan", "install", ".",
        f"--profile={profile_path}",
        "--output-folder=build",
        "--build=missing",
        "-s:b", "compiler.runtime=dynamic",
        "-s:b", "compiler.runtime_type=Release"
    ], project_root):
        print("Failed to install dependencies for Release")
        return False

    # Generate Visual Studio solution
    if not run_command([
        "cmake", "..",
        "-G", "Visual Studio 17 2022",
        "-DCMAKE_TOOLCHAIN_FILE=conan/conan_toolchain.cmake",
        "-DCMAKE_BUILD_TYPE=Release"
    ], build_dir):
        print("Failed to generate Visual Studio solution")
        return False
    
    # Clean up temporary profiles
    shutil.rmtree(temp_profiles_dir)
    
    print("\nSetup completed successfully!")
    print(f"\nYou can now open {build_dir}/AniStudio.sln in Visual Studio")
    print("Release configuration is ready to use.")
    return True

if __name__ == "__main__":
    vs_path = find_visual_studio()
    if vs_path is None:
        print("Error: Could not find Visual Studio 2022 installation")
        sys.exit(1)
    
    print(f"Using Visual Studio at: {vs_path}")
    
    if not setup_project():
        sys.exit(1)
    sys.exit(0)